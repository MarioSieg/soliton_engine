// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "graphics_subsystem.hpp"

#include "../platform/platform_subsystem.hpp"
#include "../scripting/convar.hpp"

#include <mimalloc.h>

#include "material.hpp"
#include "shader_registry.hpp"
#include "pipeline.hpp"

#include "pipelines/pbr_pipeline.hpp"
#include "pipelines/sky.hpp"

using platform::platform_subsystem;

namespace graphics {
    using vkb::context;

    static convar<std::string> cv_shader_dir {"Renderer.shaderDir", std::nullopt, scripting::convar_flags::read_only};
    static convar<bool> cv_enable_parallel_shader_compilation {"Renderer.enableParallelShaderCompilation", true, scripting::convar_flags::read_only};
    static convar<std::uint32_t> cv_max_render_threads {
        "Threads.renderThreads",
        2u,
        scripting::convar_flags::read_only,
        1u,
        std::max(1u, std::thread::hardware_concurrency())
    };

    graphics_subsystem::graphics_subsystem() : subsystem{"Graphics"} {
        log_info("Initializing graphics subsystem");

        s_instance = this;

        GLFWwindow* window = platform_subsystem::get_glfw_window();
        context::init(window);

        material::init_static_resources();

        create_descriptor_pool();

        shader_registry::init(cv_shader_dir());
        if (!shader_registry::get().compile_all(cv_enable_parallel_shader_compilation())) [[unlikely]] {
           log_error("Failed to compile shaders");
        }

        pipeline_registry::init();
        auto& reg = pipeline_registry::get();
        reg.register_pipeline<pipelines::pbr_pipeline>();
        reg.register_pipeline<pipelines::sky_pipeline>();

        m_render_thread_pool.emplace(&render_scene_bucket, this, cv_max_render_threads());
        m_render_data.reserve(32);

        m_imgui_context.emplace();

        m_noesis_context.emplace();

        //m_noesis_context->load_ui_from_xaml("App.xaml");
    }

    graphics_subsystem::~graphics_subsystem() {
        vkcheck(vkb::vkdvc().waitIdle()); // must be first

        m_imgui_context.reset();
        m_noesis_context.reset();

        shader_registry::shutdown();
        pipeline_registry::shutdown();
        m_render_thread_pool.reset();
        if (m_debugdraw) {
            m_debugdraw.reset();
        }
        material::free_static_resources();
        context::shutdown();
        s_instance = nullptr;
    }

    [[nodiscard]] static auto compute_render_bucket_range(const std::size_t id, const std::size_t num_entities, const std::size_t num_threads) noexcept -> std::array<std::size_t, 2> {
        const std::size_t base_bucket_size = num_entities/num_threads;
        const std::size_t num_extra_entities = num_entities%num_threads;
        const std::size_t begin = base_bucket_size*id + std::min(id, num_extra_entities);
        const std::size_t end = begin + base_bucket_size + (id < num_extra_entities ? 1 : 0);
        passert(begin <= end && end <= num_entities);
        return {begin, end};
    }

    [[nodiscard]] static auto find_main_camera() -> flecs::entity {
        const auto filter = scene::get_active().filter<const com::transform, const com::camera>();
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
        com::camera::active_camera = main_cam;
        s_camera_transform = *main_cam.get<com::transform>();
        com::camera& cam = *main_cam.get_mut<com::camera>();
        if (cam.auto_viewport) {
            cam.viewport.x = width;
            cam.viewport.y = height;
        }
        DirectX::XMStoreFloat4A(&s_clear_color, DirectX::XMVectorSetW(DirectX::XMLoadFloat3(&cam.clear_color), 1.0f));
        const DirectX::XMMATRIX view = cam.compute_view(s_camera_transform);
        const DirectX::XMMATRIX proj = cam.compute_projection();
        DirectX::XMStoreFloat4x4A(&s_view_mtx, view);
        DirectX::XMStoreFloat4x4A(&s_proj_mtx, proj);
        DirectX::XMStoreFloat4x4A(&s_view_proj_mtx, DirectX::XMMatrixMultiply(view, proj));
        DirectX::BoundingFrustum::CreateFromMatrix(s_frustum, proj);
        s_frustum.Transform(s_frustum, DirectX::XMMatrixInverse(nullptr, view));
    }

    // WARNING! RENDER THREAD LOCAL
    HOTPROC auto graphics_subsystem::render_scene_bucket(
        const vk::CommandBuffer cmd,
        const std::int32_t bucket_id,
        const std::int32_t num_threads,
        void* const usr
    ) -> void {
        passert(usr != nullptr);
        auto& self = *static_cast<graphics_subsystem*>(usr);

        const auto& pbr_pipeline = dynamic_cast<const pipelines::pbr_pipeline&>(pipeline_registry::get().get_pipeline("pbr"));

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pbr_pipeline.get_pipeline());
        const DirectX::XMMATRIX vp = DirectX::XMLoadFloat4x4A(&graphics_subsystem::s_view_proj_mtx);

        // thread workload distribution
        const std::vector<std::pair<std::span<const com::transform>, std::span<const com::mesh_renderer>>>& render_data = self.get_render_data();
        const std::size_t total_entities = std::accumulate(render_data.cbegin(), render_data.cend(), 0, [](const std::size_t acc, const auto& pair) noexcept {
            passert(pair.first.size() == pair.second.size());
            return acc + pair.first.size();
        });

        // Compute start and end for this thread across all entities
        const auto [global_start, global_end] = compute_render_bucket_range(bucket_id, total_entities, num_threads);
        std::size_t processed_entities = 0;
        for (const auto& [transforms, renderers] : render_data) {
            if (processed_entities >= global_end) break; // Already processed all entities this thread is responsible for
            std::size_t local_start = 0;
            std::size_t local_end = transforms.size();
            if (processed_entities < global_start)
                local_start = std::min(transforms.size(), global_start - processed_entities);
            if (processed_entities + transforms.size() > global_end)
                local_end = std::min(transforms.size(), global_end - processed_entities);
            for (std::size_t i = local_start; i < local_end; ++i)
                pbr_pipeline.render_mesh(cmd, pbr_pipeline.get_layout(), transforms[i], renderers[i], s_frustum, vp);
            processed_entities += transforms.size();
        }

        if (bucket_id == num_threads - 1) { // Last thread
            if (std::optional<debugdraw>& dd = self.get_debug_draw_opt(); dd) {
                dd->render(
                    cmd,
                    DirectX::XMLoadFloat4x4A(&graphics_subsystem::s_view_proj_mtx),
                    DirectX::XMLoadFloat4(&graphics_subsystem::s_camera_transform.position)
                );
            }
            self.get_imgui_context().submit_imgui(cmd);
        }
    }

    HOTPROC auto graphics_subsystem::on_pre_tick() -> bool {
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
        m_cmd = vkb::ctx().begin_frame(s_clear_color, &m_inheritance_info);
        vkb::ctx().begin_render_pass(m_cmd, vkb::ctx().get_scene_render_pass(), vk::SubpassContents::eSecondaryCommandBuffers);
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
        scene.readonly_begin();
        m_render_data.clear();
        m_render_query.query.iter([this](const flecs::iter& i, const com::transform* transforms, const com::mesh_renderer* renderers) {
            const std::size_t n = i.count();
            m_render_data.emplace_back(std::span{transforms, n}, std::span{renderers, n});
        });
        if (m_cmd) [[likely]] { // TODO: refractor
            m_render_thread_pool->begin_frame(&m_inheritance_info);
            m_render_thread_pool->process_frame(m_cmd);
            scene.readonly_end();
            vkb::ctx().end_render_pass(m_cmd);
            vkcheck(m_cmd.end());
            m_render_data.clear();

            //render_uis(); TODO

            vkb::ctx().end_frame(m_cmd);
        }
        com::camera::active_camera = flecs::entity::null();
    }

    auto graphics_subsystem::on_resize() -> void {
        vkb::ctx().on_resize();
        m_noesis_context->on_resize();
    }

    auto graphics_subsystem::on_start(scene& scene) -> void {
        
    }

    // Descriptors are allocated from a pool, that tells the implementation how many and what types of descriptors we are going to use (at maximum)
    auto graphics_subsystem::create_descriptor_pool() -> void {
        const vkb::device& dvc = vkb::dvc();
        const vk::Device device = vkb::vkdvc();

        const std::array<vk::DescriptorPoolSize, 1> sizes {
            vk::DescriptorPoolSize {
                .type = vk::DescriptorType::eUniformBufferDynamic,
                .descriptorCount = 128u
            },
        };

        // Create the global descriptor pool
        // All descriptors used in this example are allocated from this pool
        vk::DescriptorPoolCreateInfo descriptor_pool_ci {};
        descriptor_pool_ci.poolSizeCount = static_cast<std::uint32_t>(sizes.size());
        descriptor_pool_ci.pPoolSizes = sizes.data();
        descriptor_pool_ci.maxSets = 32; // TODO max sets Allocate one set for each frame
        vkcheck(device.createDescriptorPool(&descriptor_pool_ci, &vkb::s_allocator, &m_descriptor_pool));
    }

    auto graphics_subsystem::render_uis() -> void {
        constexpr vk::CommandBufferBeginInfo begin_info {};
        vkcheck(m_cmd.begin(&begin_info));
        m_noesis_context->render_offscreen(m_cmd);

        auto w = static_cast<float>(vkb::ctx().get_width());
        auto h = static_cast<float>(vkb::ctx().get_height());

        // Update dynamic viewport state
        vk::Viewport viewport {};
        viewport.width = w;
        viewport.height = -h;
        viewport.x = 0.0f;
        viewport.y = h;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        m_cmd.setViewport(0, 1, &viewport);

        // Update dynamic scissor state
        vk::Rect2D scissor {};
        scissor.extent.width = w;
        scissor.extent.height = h;
        scissor.offset.x = 0.0f;
        scissor.offset.y = 0.0f;
        m_cmd.setScissor(0, 1, &scissor);

        vkb::ctx().begin_render_pass(m_cmd, vkb::ctx().get_ui_render_pass(), vk::SubpassContents::eInline);
        m_noesis_context->render_onscreen(vkb::ctx().get_ui_render_pass());
        m_imgui_context->submit_imgui(m_cmd);
        vkb::ctx().end_render_pass(m_cmd);

        vkcheck(m_cmd.end());
    }

    auto graphics_subsystem::reload_pipelines() -> void {
        log_info("Reloading pipelines");
        const auto now = std::chrono::high_resolution_clock::now();
        if (shader_registry::get().compile_all(cv_enable_parallel_shader_compilation())) [[likely]] {
            auto& reg = pipeline_registry::get();
            reg.try_recreate_all();
            log_info("Reloaded pipelines in {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - now).count());
        } else {
            log_error("Failed to compile shaders");
        }
    }
}
