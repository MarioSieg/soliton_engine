// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/subsystem.hpp"

#include "prelude.hpp"

namespace graphics {
    class graphics_subsystem final : public subsystem {
    public:
        graphics_subsystem();
        ~graphics_subsystem() override;

        auto on_pre_tick() -> bool override;
        auto on_post_tick() -> void override;
        auto on_resize() -> void override;

        [[nodiscard]] static auto is_draw_phase() noexcept -> bool { return s_is_draw_phase; } // is draw phase?
        [[nodiscard]] static auto is_dd_init() noexcept -> bool { return s_is_dd_init; } // is debug draw initialized?
        static auto init_debug_draw_lazy() -> void; // initialize debug draw if not already initialized

        [[nodiscard]] auto get_width() const noexcept -> float { return m_width; }
        [[nodiscard]] auto get_height() const noexcept -> float { return m_height; }
        [[nodiscard]] auto get_reset_flags() const noexcept -> std::uint32_t { return m_reset_flags; }

        static constexpr bgfx::ViewId k_scene_view = 0;
        static constexpr bgfx::ViewId k_imgui_view = 0xff;

    private:
        static constinit inline bool s_is_draw_phase = false; // is draw phase?
        static constinit inline bool s_is_dd_init = false; // is debug draw initialized?
        float m_width = 0.0f;
        float m_height = 0.0f;
        handle<bgfx::ProgramHandle> m_program {};
        handle<bgfx::VertexBufferHandle> m_vb {};
        handle<bgfx::IndexBufferHandle> m_ib {};
        std::uint32_t m_reset_flags = 0;
    };
}
