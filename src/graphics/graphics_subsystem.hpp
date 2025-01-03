// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/subsystem.hpp"
#include "../core/fs_watchdog.hpp"
#include "../scene/scene.hpp"

#include "vulkancore/context.hpp"
#include "vulkancore/buffer.hpp"
#include "vulkancore/command_buffer.hpp"

#include "render_thread_pool.hpp"
#include "shader.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "utils/debugdraw.hpp"
#include "shared_buffers.hpp"
#include "pipeline_cache.hpp"
#include "imgui/context.hpp"

namespace soliton::graphics {
    class graphics_subsystem final : public subsystem {
    public:
        graphics_subsystem();
        ~graphics_subsystem();

        HOTPROC auto on_pre_tick() -> bool override;
        HOTPROC auto on_post_tick() -> void override;
        auto on_resize() -> void override;
        auto on_start(scene& scene) -> void override;

        [[nodiscard]] auto get_render_thread_pool() const noexcept -> const render_thread_pool& { return *m_render_thread_pool; }

        [[nodiscard]] auto get_render_data() const noexcept -> const eastl::vector<eastl::pair<eastl::span<const com::transform>, eastl::span<const com::mesh_renderer>>>& { return m_render_data; }
        [[nodiscard]] auto get_debug_draw() -> debugdraw& {
            if (!m_debugdraw.has_value()) {
                m_debugdraw.emplace();
            }
            return *m_debugdraw;
        }

        [[nodiscard]] auto get_debug_draw_opt() noexcept -> eastl::optional<debugdraw>& { return m_debugdraw; }
        [[nodiscard]] static auto get() noexcept -> graphics_subsystem& {
            panic_assert(s_instance != nullptr);
            return *s_instance;
        }
        [[nodiscard]] static auto get_num_draw_calls() noexcept -> std::uint32_t;
        [[nodiscard]] static auto get_num_draw_verts() noexcept -> std::uint32_t;
        [[nodiscard]] static auto get_view_mtx() noexcept -> const XMFLOAT4X4A& { return s_view_mtx; }
        [[nodiscard]] static auto get_proj_mtx() noexcept -> const XMFLOAT4X4A& { return s_proj_mtx; }
        [[nodiscard]] static auto get_view_proj_mtx() noexcept -> const XMFLOAT4X4A& { return s_view_proj_mtx; }
        [[nodiscard]] static auto get_frustum() noexcept -> const BoundingFrustum& { return s_frustum; }
        [[nodiscard]] static auto get_clear_color() noexcept -> XMFLOAT4A& { return s_clear_color; }
        [[nodiscard]] static auto get_camera_transform() noexcept -> com::transform& { return s_camera_transform; }

        auto hot_reload_pipelines() noexcept -> void {
            m_reload_pipelines_next_frame = true;
        }

    private:
        friend class graphics_pipeline;

        eastl::optional<fs_watchdog> m_shader_reload_watcher {};
        bool m_reload_pipelines_next_frame {};

        eastl::optional<pipeline_cache> m_pipeline_cache {};
        eastl::optional<vkb::command_buffer> m_cmd {};
        vk::CommandBufferInheritanceInfo m_inheritance_info {};
        eastl::optional<render_thread_pool> m_render_thread_pool {};
        eastl::vector<eastl::pair<eastl::span<const com::transform>, eastl::span<const com::mesh_renderer>>> m_render_data {};
        eastl::optional<debugdraw> m_debugdraw {};
        eastl::optional<imgui::context> m_imgui_context {};

        static inline XMFLOAT4X4A s_view_mtx;
        static inline XMFLOAT4X4A s_proj_mtx;
        static inline XMFLOAT4X4A s_view_proj_mtx;
        static inline XMFLOAT4A s_clear_color;
        static inline BoundingFrustum s_frustum;
        static inline com::transform s_camera_transform;
        static inline float s_camera_fov;
        static inline graphics_subsystem* s_instance;

        auto render_uis() -> void;
        auto load_all_pipelines(bool force_success) -> void;
        auto update_shared_buffers_per_frame() const -> void;
        static auto update_main_camera(float width, float height) -> void;
        HOTPROC auto render_scene_bucket(
            vkb::command_buffer& cmd,
            std::int32_t bucket_id,
            std::int32_t num_threads
        ) const -> void;
    };
}
