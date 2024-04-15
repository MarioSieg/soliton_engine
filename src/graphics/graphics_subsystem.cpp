// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "graphics_subsystem.hpp"

#include "../platform/platform_subsystem.hpp"
#include "../scripting/scripting_subsystem.hpp"

#include <execution>
#include <mimalloc.h>

#include "imgui/text_editor.hpp"
#include "imgui/implot.h"
#include "material.hpp"
#include "pipeline.hpp"
#include "RmlUi/Core.h"

#include "pipelines/pbr_pipeline.hpp"

using platform::platform_subsystem;
using scripting::scripting_subsystem;

namespace graphics {
    using vkb::context;

    static auto render_scene_bucket(vk::CommandBuffer cmd_buf, std::int32_t bucket_id, std::int32_t num_threads, void* usr) -> void;

    graphics_subsystem::graphics_subsystem() : subsystem{"Graphics"} {
        log_info("Initializing graphics subsystem");

        s_instance = this;
        ImGui::SetAllocatorFunctions(
            +[](size_t size, [[maybe_unused]] void* usr) -> void* {
                return mi_malloc(size);
            },
            +[](void* ptr, [[maybe_unused]] void* usr) -> void {
                mi_free(ptr);
            }
        );
        ImGui::CreateContext();
        ImPlot::CreateContext();
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = 800.0f;
        io.DisplaySize.y = 600.0f;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.IniFilename = nullptr;

        GLFWwindow* window = platform_subsystem::get_glfw_window();
        context::s_instance = std::make_unique<context>(window); // Create Vulkan context

        // Apply DPI scaling
        float scale = 1.0f;
        float xscale;
        float yscale;
        glfwGetWindowContentScale(window, &xscale, &yscale);
        scale = (xscale + yscale) * 0.5f;
        if constexpr (PLATFORM_OSX) {
            io.FontGlobalScale = 1.0f / scale;
        } else {
            ImGui::GetStyle().ScaleAllSizes(scale);
        }

        material::init_static_resources();

        create_descriptor_pool();

        pipeline_registry::s_instance = std::make_unique<pipeline_registry>(vkb_context().get_device().get_logical_device());

        const auto num_render_threads = scripting_subsystem::get_config_table()["Threads"]["renderThreads"].cast<std::int32_t>().valueOr(2);
        m_render_thread_pool.emplace(&render_scene_bucket, this, num_render_threads);
        m_render_data.reserve(32);

        pipeline_registry::get().register_pipeline<pipelines::pbr_pipeline>();

        init_rmlui();
    }

    [[nodiscard]] static auto compute_render_bucket_range(const std::size_t id, const std::size_t num_entities, const std::size_t num_threads) noexcept -> std::array<std::size_t, 2> {
        const std::size_t base_bucket_size = num_entities / num_threads;
        const std::size_t num_extra_entities = num_entities % num_threads;
        const std::size_t begin = base_bucket_size * id + std::min(id, num_extra_entities);
        const std::size_t end = begin + base_bucket_size + (id < num_extra_entities ? 1 : 0);
        passert(begin <= end && end <= num_entities);
        return {begin, end};
    }

    graphics_subsystem::~graphics_subsystem() {
        shutdown_rmlui();
        pipeline_registry::s_instance.reset();
        m_render_thread_pool.reset();
        vkcheck(context::s_instance->get_device().get_logical_device().waitIdle());
        if (m_debugdraw) {
            m_debugdraw.reset();
        }
        material::free_static_resources();
        context::s_instance.reset();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        s_instance = nullptr;
    }

    [[nodiscard]] static auto find_main_camera() -> flecs::entity {
        const auto filter = scene::get_active().filter<const com::transform, const com::camera>();
        if (filter.count() > 0) {
            return filter.first();
        }
        return flecs::entity::null();
    }

    static auto update_main_camera(const float width, const float height) -> void {
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
        graphics_subsystem::s_camera_transform = *main_cam.get<com::transform>();
        com::camera& cam = *main_cam.get_mut<com::camera>();
        if (cam.auto_viewport) {
            cam.viewport.x = width;
            cam.viewport.y = height;
        }
        DirectX::XMStoreFloat4A(&graphics_subsystem::s_clear_color, DirectX::XMVectorSetW(DirectX::XMLoadFloat3(&cam.clear_color), 1.0f));
        const DirectX::XMMATRIX view = cam.compute_view(graphics_subsystem::s_camera_transform);
        const DirectX::XMMATRIX proj = cam.compute_projection();
        DirectX::XMStoreFloat4x4A(&graphics_subsystem::s_view_mtx, view);
        DirectX::XMStoreFloat4x4A(&graphics_subsystem::s_proj_mtx, proj);
        DirectX::XMStoreFloat4x4A(&graphics_subsystem::s_view_proj_mtx, DirectX::XMMatrixMultiply(view, proj));
        DirectX::BoundingFrustum::CreateFromMatrix(graphics_subsystem::s_frustum, proj);
        graphics_subsystem::s_frustum.Transform(graphics_subsystem::s_frustum, DirectX::XMMatrixInverse(nullptr, view));
    }

    // WARNING! RENDER THREAD LOCAL
    HOTPROC static auto draw_mesh(
        const mesh& mesh,
        const vk::CommandBuffer cmd,
        const std::vector<material*>& mats,
        const vk::PipelineLayout layout
    ) -> void {
        constexpr vk::DeviceSize offsets = 0;
        cmd.bindIndexBuffer(mesh.get_index_buffer().get_buffer(), 0, mesh.is_index_32bit() ? vk::IndexType::eUint32 : vk::IndexType::eUint16);
        cmd.bindVertexBuffers(0, 1, &mesh.get_vertex_buffer().get_buffer(), &offsets);
        if (mesh.get_primitives().size() <= mats.size()) { // we have at least one material for each primitive
            for (std::size_t idx = 0; const mesh::primitive& prim : mesh.get_primitives()) {
                cmd.bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,
                    layout,
                    1,
                    1,
                    &mats[idx++]->get_descriptor_set(),
                    0,
                    nullptr
                );
                cmd.drawIndexed(prim.index_count, 1, prim.index_start, 0, 1);
            }
        } else {
            cmd.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                layout,
                1,
                1,
                &mats[0]->get_descriptor_set(),
                0,
                nullptr
            );
            cmd.drawIndexed(mesh.get_index_count(), 1, 0, 0, 0);
        }
    }

    // WARNING! RENDER THREAD LOCAL
    HOTPROC static auto render_mesh(
        const vk::CommandBuffer cmd_buf,
        const vk::PipelineLayout layout,
        const com::transform& transform,
        const com::mesh_renderer& renderer,
        DirectX::FXMMATRIX vp
    ) -> void {
        if (renderer.meshes.empty() || renderer.materials.empty()) [[unlikely]] {
            log_warn("Mesh renderer has no meshes or materials");
            return;
        }
        if (renderer.flags & com::render_flags::skip_rendering) [[unlikely]] {
            return;
        }
        const DirectX::XMMATRIX model = transform.compute_matrix();
        for (const mesh* mesh : renderer.meshes) {
            if (!mesh) [[unlikely]] {
                log_warn("Mesh renderer has a null mesh");
                continue;
            }

            // Frustum Culling
            DirectX::BoundingOrientedBox obb {};
            obb.CreateFromBoundingBox(obb, mesh->get_aabb());
            obb.Transform(obb, model);
            if ((renderer.flags & com::render_flags::skip_frustum_culling) == 0) [[likely]] {
                if (graphics_subsystem::s_frustum.Contains(obb) == DirectX::ContainmentType::DISJOINT) { // Object is culled
                    return;
                }
            }

            // Uniforms
            pipelines::pbr_pipeline::gpu_vertex_push_constants push_constants {};
            DirectX::XMStoreFloat4x4A(&push_constants.model_view_proj, DirectX::XMMatrixMultiply(model, vp));
            DirectX::XMStoreFloat4x4A(&push_constants.normal_matrix, DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, model)));
            cmd_buf.pushConstants(
                layout,
                vk::ShaderStageFlagBits::eVertex,
                0,
                sizeof(push_constants),
                &push_constants
            );

            draw_mesh(*mesh, cmd_buf, renderer.materials, layout);
        }
    }

    // WARNING! RENDER THREAD LOCAL
    HOTPROC static auto render_scene_bucket(const vk::CommandBuffer cmd_buf, const std::int32_t bucket_id, const std::int32_t num_threads, void* usr) -> void {
        passert(usr != nullptr);
        auto& self = *static_cast<graphics_subsystem*>(usr);

        const pipeline_base& pipe = pipeline_registry::get().get_pipeline("pbr");

        cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipe.get_pipeline());
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
            if (processed_entities < global_start) {
                local_start = std::min(transforms.size(), global_start - processed_entities);
            }
            if (processed_entities + transforms.size() > global_end) {
                local_end = std::min(transforms.size(), global_end - processed_entities);
            }
            for (std::size_t i = local_start; i < local_end; ++i) {
                render_mesh(cmd_buf, pipe.get_layout(), transforms[i], renderers[i], vp);
            }
            processed_entities += transforms.size();
        }

        if (bucket_id == num_threads - 1) { // Last thread
            if (std::optional<debugdraw>& dd = self.get_debug_draw_opt(); dd) {
                dd->render(
                    cmd_buf,
                    DirectX::XMLoadFloat4x4A(&graphics_subsystem::s_view_proj_mtx),
                    DirectX::XMLoadFloat4(&graphics_subsystem::s_camera_transform.position)
                );
            }

            self.get_rmlui_renderer()->m_p_current_command_buffer = cmd_buf;
            passert(self.get_ui_context()->Render());

            vkb_context().render_imgui(ImGui::GetDrawData(), cmd_buf);
        }
    }

    HOTPROC auto graphics_subsystem::on_pre_tick() -> bool {
        const auto w = static_cast<float>(context::s_instance->get_width());
        const auto h = static_cast<float>(context::s_instance->get_height());
        ImGui::NewFrame();
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = w;
        io.DisplaySize.y = h;
        update_main_camera(w, h);
        m_cmd_buf = vkb_context().begin_frame(s_clear_color, &m_inheritance_info);
        return true;
    }

    HOTPROC auto graphics_subsystem::on_post_tick() -> void {
        ImGui::Render();
        m_ui_context->Update();
        auto& scene = scene::get_active();
        if (!m_render_query.scene || &scene != m_render_query.scene) [[unlikely]] { // Scene changed
            m_render_query.scene = &scene;
            // m_render_query.query.destruct(); TODO: leak
            m_render_query.query = scene.query<const com::transform, const com::mesh_renderer>();
        }
        scene.readonly_begin();
        m_render_data.clear();
        m_render_query.query.iter([this](const flecs::iter& i, const com::transform* transforms, const com::mesh_renderer* renderers) {
            const std::size_t n = i.count();
            m_render_data.emplace_back(std::make_pair(std::span{transforms, n}, std::span{renderers, n}));
        });
        if (m_cmd_buf) [[likely]] {
            m_render_thread_pool->begin_frame(&m_inheritance_info);
            m_render_thread_pool->process_frame(m_cmd_buf);
            m_render_data.clear();
        }
        scene.readonly_end();

        if (m_cmd_buf) [[likely]] {
            vkb_context().end_frame(m_cmd_buf);
        }
        com::camera::active_camera = flecs::entity::null();
    }

    auto graphics_subsystem::on_resize() -> void {
        vkb_context().on_resize();
        m_rmlui_renderer->SetViewport(vkb_context().get_width(), vkb_context().get_height());
    }

    auto graphics_subsystem::on_start(scene& scene) -> void {
        
    }

    // Descriptors are allocated from a pool, that tells the implementation how many and what types of descriptors we are going to use (at maximum)
    auto graphics_subsystem::create_descriptor_pool() -> void {
        const vkb::device& vkb_device = context::s_instance->get_device();
        const vk::Device device = vkb_device.get_logical_device();

        const std::array<vk::DescriptorPoolSize, 2> sizes {
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

    auto graphics_subsystem::init_rmlui() -> void {
        m_rmlui_system = std::make_unique<SystemInterface_GLFW>();
        Rml::SetSystemInterface(&*m_rmlui_system);

        m_rmlui_renderer = std::make_unique<RenderInterface_VK>();
        passert(m_rmlui_renderer->Initialize(vkb_context()));
        Rml::SetRenderInterface(&*m_rmlui_renderer);
        m_rmlui_renderer->SetViewport(1920, 1080);

        Rml::Initialise();

        Rml::LoadFontFace("assets/fonts/LatoLatin-Regular.ttf");
        Rml::LoadFontFace("assets/fonts/NotoEmoji-Regular.ttf", true);

        m_ui_context = Rml::CreateContext("main", Rml::Vector2i{1920, 1080});

        Rml::ElementDocument* document = m_ui_context->LoadDocument("assets/ui/hello_world.rml");
        document->Show();
    }

    auto graphics_subsystem::shutdown_rmlui() -> void {
        Rml::Shutdown();
        m_rmlui_renderer.reset();
        m_rmlui_system.reset();
    }
}
