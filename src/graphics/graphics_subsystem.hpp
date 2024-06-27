// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <optional>

#include "../core/subsystem.hpp"
#include "../scene/scene.hpp"

#include "vulkancore/context.hpp"
#include "shader.hpp"
#include "vulkancore/buffer.hpp"

#include "render_thread_pool.hpp"

#include "mesh.hpp"
#include "texture.hpp"
#include "debugdraw.hpp"

#include "imgui/context.hpp"
#include "noesis/context.hpp"

namespace graphics {
    class graphics_subsystem final : public subsystem {
    public:
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

        [[nodiscard]] auto get_debug_draw_opt() noexcept -> std::optional<debugdraw>& { return m_debugdraw; }
        [[nodiscard]] auto get_noesis_context() noexcept -> noesis::context& { return *m_noesis_context; }
        [[nodiscard]] auto get_imgui_context() noexcept -> imgui::context& { return *m_imgui_context; }
        [[nodiscard]] static auto get() noexcept -> graphics_subsystem& {
            passert(s_instance != nullptr);
            return *s_instance;
        }
        [[nodiscard]] static auto get_view_mtx() noexcept -> const DirectX::XMFLOAT4X4A& { return s_view_mtx; }
        [[nodiscard]] static auto get_proj_mtx() noexcept -> const DirectX::XMFLOAT4X4A& { return s_proj_mtx; }
        [[nodiscard]] static auto get_view_proj_mtx() noexcept -> const DirectX::XMFLOAT4X4A& { return s_view_proj_mtx; }
        [[nodiscard]] static auto get_frustum() noexcept -> const DirectX::BoundingFrustum& { return s_frustum; }
        [[nodiscard]] static auto get_clear_color() noexcept -> DirectX::XMFLOAT4A& { return s_clear_color; }
        [[nodiscard]] static auto get_camera_transform() noexcept -> com::transform& { return s_camera_transform; }

        auto hot_reload_pipelines() noexcept -> void {
            m_reload_pipelines_next_frame = true;
        }

    private:
        static auto reload_pipelines() -> void;
        auto create_descriptor_pool() -> void;
        auto render_uis() -> void;
        static auto update_main_camera(float width, float height) -> void;
        HOTPROC static auto render_scene_bucket(
            vk::CommandBuffer cmd,
            std::int32_t bucket_id,
            std::int32_t num_threads,
            void* usr
        ) -> void;

        static inline constinit DirectX::XMFLOAT4X4A s_view_mtx;
        static inline constinit DirectX::XMFLOAT4X4A s_proj_mtx;
        static inline constinit DirectX::XMFLOAT4X4A s_view_proj_mtx;
        static inline DirectX::XMFLOAT4A s_clear_color;
        static inline DirectX::BoundingFrustum s_frustum;
        static inline com::transform s_camera_transform;
        static inline constinit graphics_subsystem* s_instance;
        vk::CommandBuffer m_cmd = nullptr;
        vk::DescriptorPool m_descriptor_pool {};
        vk::CommandBufferInheritanceInfo m_inheritance_info {};
        std::optional<render_thread_pool> m_render_thread_pool {};
        std::vector<std::pair<std::span<const com::transform>, std::span<const com::mesh_renderer>>> m_render_data {};
        std::optional<debugdraw> m_debugdraw {};
        std::optional<imgui::context> m_imgui_context {};
        std::optional<noesis::context> m_noesis_context {};
        bool m_reload_pipelines_next_frame {};

        struct {
            flecs::query<const com::transform, const com::mesh_renderer> query {};
            scene* scene {};
        } m_render_query {};
    };
}
