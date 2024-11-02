// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "graphics_subsystem.hpp"

#include "../platform/platform_subsystem.hpp"
#include "../scripting/system_variable.hpp"

#if USE_MIMALLOC
#include <mimalloc.h>
#endif

#include "material.hpp"
#include "draw_partition.hpp"
#include "pipeline_cache.hpp"
#include "pipelines/pbr_pipeline.hpp"
#include "pipelines/sky.hpp"

namespace lu::graphics {
    using platform::platform_subsystem;
    using vkb::context;

    static const system_variable<eastl::string> cv_shader_dir {"renderer.shader_dir", eastl::monostate{}};
    static const system_variable<std::uint32_t> cv_concurrent_frames {"renderer.concurrent_frames", {3}};
    static const system_variable<std::uint32_t> cv_msaa_samples {"renderer.msaa_samples", {4}};
    static const system_variable<std::uint32_t> cv_max_render_threads {"cpu.render_threads", {2}};

    static constinit std::uint32_t s_num_draw_calls_prev = 0;
    static constinit std::uint32_t s_num_draw_verts_prev = 0;

    graphics_subsystem::graphics_subsystem() : subsystem{"Graphics"} {
        log_info("Initializing graphics subsystem");

        s_instance = this;

        GLFWwindow* const window = *platform_subsystem::get_main_window();
        context::create(vkb::context_desc {
            .window = window,
            .concurrent_frames = cv_concurrent_frames(),
            .msaa_samples = cv_msaa_samples()
        });

        shader_cache::init(cv_shader_dir()); // 1. init shader cache (must be first)
        pipeline_cache::init();
        pipeline_cache::get().register_pipeline_async<pipelines::pbr_pipeline>();
        pipeline_cache::get().register_pipeline_async<pipelines::sky_pipeline>();

        material::init_static_resources();

        m_render_thread_pool.emplace([this](vkb::command_buffer& cmd, const std::int32_t bucket_id, const std::int32_t num_threads) -> void {
            render_scene_bucket(cmd, bucket_id, num_threads);
        }, cv_max_render_threads());
        m_render_data.reserve(32);

        m_imgui_context.emplace();
        m_noesis_context.emplace();

        //m_noesis_context->load_ui_from_xaml("App.xaml");
    }

    auto graphics_subsystem::on_prepare() -> void {
        pipeline_cache::get().await_all_pipelines_async();
    }

    graphics_subsystem::~graphics_subsystem() {
        vkcheck(vkb::vkdvc().waitIdle()); // must be first

        shared_buffers::get().~shared_buffers();

        m_imgui_context.reset();
        m_noesis_context.reset();

        shader_cache::shutdown();
        pipeline_cache::shutdown();
        m_render_thread_pool.reset();
        if (m_debugdraw) {
            m_debugdraw.reset();
        }
        material::free_static_resources();
        context::shutdown();
        s_instance = nullptr;
    }

    [[nodiscard]] static auto find_main_camera() -> flecs::entity {
        const auto filter = scene::get_active().query<const com::transform, const com::camera>();
        if (filter.count() > 0) {
            return filter.first();
        }
        return flecs::entity::null();
    }

    auto graphics_subsystem::update_main_camera(const float width, const float height) -> void {
        flecs::entity main_cam;
        auto& active = scene::get_active().active_camera;
        if (!active.is_valid() || !active.is_alive()) {
            active = flecs::entity::null();
            main_cam = find_main_camera();
        } else {
            main_cam = active;
        }
        if (!main_cam.is_valid()
            || !main_cam.is_alive()
            || !main_cam.has<const com::camera>()
            || !main_cam.has<const com::transform>()) [[unlikely]] {
                log_warn("No camera found in scene");
                return;
            }
        s_camera_transform = *main_cam.get<com::transform>();
        com::camera& cam = *main_cam.get_mut<com::camera>();
        if (cam.auto_viewport) {
            cam.viewport.x = width;
            cam.viewport.y = height;
        }
        XMStoreFloat4A(&s_clear_color, XMVectorSetW(XMLoadFloat3(&cam.clear_color), 1.0f));
        const XMMATRIX view = cam.compute_view(s_camera_transform);
        const XMMATRIX proj = cam.compute_projection();
        XMStoreFloat4x4A(&s_view_mtx, view);
        XMStoreFloat4x4A(&s_proj_mtx, proj);
        XMStoreFloat4x4A(&s_view_proj_mtx, XMMatrixMultiply(view, proj));
        BoundingFrustum::CreateFromMatrix(s_frustum, proj);
        s_frustum.Transform(s_frustum, XMMatrixInverse(nullptr, view));
    }

    // WARNING! RENDER THREAD LOCAL
    HOTPROC auto graphics_subsystem::render_scene_bucket(
        vkb::command_buffer& cmd,
        const std::int32_t bucket_id,
        const std::int32_t num_threads
    ) const -> void {
        if (is_first_thread(bucket_id, num_threads)) {
            const auto& sky_pipeline
                = pipeline_cache::get().get_pipeline<pipelines::sky_pipeline>("sky");
            sky_pipeline.on_bind(cmd);
            sky_pipeline.render_sky(cmd);
        }

        const auto& pbr_pipeline
            = pipeline_cache::get().get_pipeline<pipelines::pbr_pipeline>("mat_pbr");
        pbr_pipeline.on_bind(cmd);

        const XMMATRIX view_mtx = XMLoadFloat4x4A(&s_view_mtx);
        const XMMATRIX view_proj_mtx = XMLoadFloat4x4A(&graphics_subsystem::s_view_proj_mtx);

        // thread workload distribution
        partitioned_draw(bucket_id, num_threads, get_render_data(), [&](
            const std::size_t i,
            const com::transform& transform,
            const com::mesh_renderer& renderer
        ) -> void {
            pbr_pipeline.render_mesh_renderer(cmd, transform, renderer, s_frustum, view_proj_mtx, view_mtx);
        });

        if (is_last_thread(bucket_id, num_threads)) { // Last thread
            if (auto& dd = const_cast<eastl::optional<debugdraw>&>(m_debugdraw); dd.has_value()) {
                const auto view_pos = XMLoadFloat4(&graphics_subsystem::s_camera_transform.position);
                dd->render(cmd, view_proj_mtx, view_pos);
            }
            if (auto& imgui = const_cast<eastl::optional<imgui::context>&>(m_imgui_context); imgui.has_value()) {
                imgui->submit_imgui(cmd);
            }
        }
    }

    HOTPROC auto graphics_subsystem::on_pre_tick() -> bool {
        auto& num_draw_calls = const_cast<std::atomic_uint32_t&>(vkb::command_buffer::get_total_draw_calls());
        auto& num_draw_verts = const_cast<std::atomic_uint32_t&>(vkb::command_buffer::get_total_draw_verts());
        s_num_draw_verts_prev = num_draw_calls.exchange(0, std::memory_order_relaxed);
        s_num_draw_calls_prev = num_draw_verts.exchange(0, std::memory_order_relaxed);
        if (m_reload_pipelines_next_frame) [[unlikely]] {
            vkcheck(vkb::vkdvc().waitIdle());
            reload_pipelines();
            vkcheck(vkb::vkdvc().waitIdle());
            m_reload_pipelines_next_frame = false;
        }
        const auto w = static_cast<float>(vkb::ctx().get_width());
        const auto h = static_cast<float>(vkb::ctx().get_height());
        m_imgui_context->begin_frame();
        update_main_camera(w, h);
        m_cmd.reset();
        m_cmd = vkb::ctx().begin_frame(s_clear_color, &m_inheritance_info);
        m_cmd->begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        vkb::ctx().begin_render_pass(*m_cmd, vkb::ctx().get_scene_render_pass(), vk::SubpassContents::eSecondaryCommandBuffers);
        update_shared_buffers_per_frame();
        return true;
    }

    HOTPROC auto graphics_subsystem::on_post_tick() -> void {
        auto& scene = scene::get_active();
        m_noesis_context->tick();
        m_imgui_context->end_frame();
        if (!m_render_query.scene || &scene != m_render_query.scene) [[unlikely]] { // Scene changed
            m_render_query.scene = &scene;
            // m_render_query.query.destruct(); TODO: leak
            m_render_query.query = scene.query<const com::transform, const com::mesh_renderer>();
        }
        m_render_data.clear();
        scene.readonly_begin();
        const std::size_t n = m_render_query.query.count();
        m_render_query.query.each([this, n](const com::transform& transform, const com::mesh_renderer& renderer) {
            m_render_data.emplace_back(eastl::span{&transform, n}, eastl::span{&renderer, n});
        });
        if (m_cmd) [[likely]] { // TODO: refractor
            m_render_thread_pool->begin_frame(&m_inheritance_info);
            m_render_thread_pool->process_frame(*m_cmd);
            scene.readonly_end();
            m_cmd->end_render_pass();
            m_render_data.clear();

            //render_uis(); TODO

            m_cmd->end();
            vkb::ctx().end_frame(*m_cmd);
        } else {
            scene.readonly_end();
        }
    }

    auto graphics_subsystem::on_resize() -> void {
        vkb::ctx().on_resize();
        m_noesis_context->on_resize();
    }

    auto graphics_subsystem::on_start(scene& scene) -> void {
        
    }

    auto graphics_subsystem::render_uis() -> void {
        m_cmd->begin();
        m_noesis_context->render_offscreen(*m_cmd);

        auto w = static_cast<float>(vkb::ctx().get_width());
        auto h = static_cast<float>(vkb::ctx().get_height());

        // Update dynamic viewport state
        m_cmd->set_viewport(0.0f, h, w, -h);
        m_cmd->set_scissor(w, h);

        vkb::ctx().begin_render_pass(*m_cmd, vkb::ctx().get_ui_render_pass(), vk::SubpassContents::eInline); // TODO refractor
        m_noesis_context->render_onscreen(vkb::ctx().get_ui_render_pass());
        m_imgui_context->submit_imgui(*m_cmd);
        m_cmd->end_render_pass();
        m_cmd->end();
    }

    auto graphics_subsystem::reload_pipelines() -> void {
        log_info("Reloading pipelines");
        const auto now = eastl::chrono::high_resolution_clock::now();
        if (true) [[likely]] { // TODO
            auto& reg = pipeline_cache::get();
            reg.recreate_all();
            log_info("Reloaded pipelines in {}ms", eastl::chrono::duration_cast<eastl::chrono::milliseconds>(eastl::chrono::high_resolution_clock::now() - now).count());
        } else {
            log_error("Failed to compile shaders");
        }
    }

    auto graphics_subsystem::get_num_draw_calls() noexcept -> std::uint32_t {
        return s_num_draw_calls_prev;
    }

    auto graphics_subsystem::get_num_draw_verts() noexcept -> std::uint32_t {
        return s_num_draw_verts_prev;
    }

    auto graphics_subsystem::update_shared_buffers_per_frame() const -> void {
        const auto& scene = scene::get_active();

        glsl::perFrameData per_frame_data {};
        XMStoreFloat4(&per_frame_data.camPos, XMLoadFloat4(&s_camera_transform.position));
        per_frame_data.sunDir = scene.properties.environment.sun_dir;
        per_frame_data.sunColor = scene.properties.environment.sun_color;
        shared_buffers::get().per_frame_ubo.set(per_frame_data);
    }
}
