// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <optional>

#include "../core/subsystem.hpp"
#include "../scene/scene.hpp"

#include "vulkancore/context.hpp"
#include "vulkancore/shader.hpp"
#include "vulkancore/buffer.hpp"

#include "render_thread_pool.hpp"

#include "mesh.hpp"
#include "texture.hpp"
#include "debugdraw.hpp"

#include "rmlui/rmlui_renderer.hpp"
#include "rmlui/rmlui_system.hpp"

namespace graphics {
    class graphics_subsystem final : public subsystem {
    public:
        static constexpr std::uint32_t k_max_concurrent_frames = 3;
        graphics_subsystem();
        ~graphics_subsystem();

        HOTPROC auto on_pre_tick() -> bool override;
        HOTPROC auto on_post_tick() -> void override;
        auto on_resize() -> void override;
        auto on_start(scene& scene) -> void override;

        [[nodiscard]] auto get_descriptor_pool() const noexcept -> vk::DescriptorPool { return m_descriptor_pool; }
        [[nodiscard]] auto get_render_thread_pool() const noexcept -> const render_thread_pool& { return *m_render_thread_pool; }

        [[nodiscard]] auto get_render_data() const noexcept -> const std::vector<std::pair<std::span<const com::transform>, std::span<const com::mesh_renderer>>>& { return m_render_data; }
        [[nodiscard]] auto get_debug_draw() -> debugdraw& {
            if (!m_debugdraw.has_value()) {
                m_debugdraw.emplace(m_descriptor_pool);
            }
            return *m_debugdraw;
        }

        [[nodiscard]] auto get_debug_draw_opt() noexcept -> std::optional<debugdraw>& {
            return m_debugdraw;
        }

        static inline constinit DirectX::XMFLOAT4X4A s_view_mtx;
        static inline constinit DirectX::XMFLOAT4X4A s_proj_mtx;
        static inline constinit DirectX::XMFLOAT4X4A s_view_proj_mtx;
        static inline DirectX::XMFLOAT4A s_clear_color;
        static inline DirectX::BoundingFrustum s_frustum;
        static inline com::transform s_camera_transform;
        static inline constinit graphics_subsystem* s_instance;

        [[nodiscard]] auto get_ui_context() const noexcept -> Rml::Context* { return m_ui_context; }
        [[nodiscard]] auto get_rmlui_system() const noexcept -> SystemInterface_GLFW* { return &*m_rmlui_system; }
        [[nodiscard]] auto get_rmlui_renderer() const noexcept -> RenderInterface_VK* { return &*m_rmlui_renderer; }

    private:
        auto create_descriptor_pool() -> void;
        auto init_rmlui() -> void;
        auto shutdown_rmlui() -> void;

        vk::CommandBuffer m_cmd_buf = nullptr;
        vk::DescriptorPool m_descriptor_pool {};
        vk::CommandBufferInheritanceInfo m_inheritance_info {};
        std::optional<render_thread_pool> m_render_thread_pool {};
        std::vector<std::pair<std::span<const com::transform>, std::span<const com::mesh_renderer>>> m_render_data {};
        std::optional<debugdraw> m_debugdraw {};
        Rml::Context* m_ui_context {};
        std::unique_ptr<SystemInterface_GLFW> m_rmlui_system {};
        std::unique_ptr<RenderInterface_VK> m_rmlui_renderer {};

        struct {
            flecs::query<const com::transform, const com::mesh_renderer> query {};
            scene* scene {};
        } m_render_query {};
    };
}
