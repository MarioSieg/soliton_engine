// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/subsystem.hpp"
#include "../scene/scene.hpp"

#include "prelude.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "pbr.hpp"

namespace graphics {
    class allocator final : public bx::AllocatorI {
    public:
        auto realloc(void* p, size_t size, size_t align, const char* filePath, std::uint32_t line) -> void* override;
    };

    struct material final {
        std::unique_ptr<texture> m_albedo;
        std::unique_ptr<texture> m_normal;
        std::unique_ptr<texture> m_metallic;
        std::unique_ptr<texture> m_roughness;
        std::unique_ptr<texture> m_ao;
    };

    class graphics_subsystem final : public subsystem {
    public:
        graphics_subsystem();
        ~graphics_subsystem() override;

        HOTPROC auto on_pre_tick() -> bool override;
        HOTPROC auto on_post_tick() -> void override;
        auto on_resize() -> void override;
        auto on_start(scene& scene) -> void override;

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
        ankerl::unordered_dense::map<std::string, handle<bgfx::ProgramHandle>> m_programs {};
        std::uint32_t m_reset_flags = BGFX_RESET_SRGB_BACKBUFFER | BGFX_RESET_MSAA_X4;
        handle<bgfx::UniformHandle> m_sampler {};
        std::optional<mesh> m_mesh {};
        std::vector<material> m_materials {};
        std::optional<pbr_renderer> m_pbr {};
    };
}
