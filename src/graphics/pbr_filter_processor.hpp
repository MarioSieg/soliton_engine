// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"
#include "vulkancore/command_buffer.hpp"

#include "mesh.hpp"
#include "texture.hpp"

namespace soliton::graphics {
    class shader_cache;

    class pbr_filter_processor final : public no_copy, public no_move {
    public:
        static constexpr vk::Format k_brfd_lut_format = vk::Format::eR16G16Sfloat;
        static constexpr vk::Format k_irradiance_cube_format = vk::Format::eR32G32B32A32Sfloat;

        explicit pbr_filter_processor(const std::shared_ptr<shader_cache>& shader_cache);

        [[nodiscard]] inline auto environ_cube() const -> const texture& { return *m_environ_cube; }
        [[nodiscard]] inline auto irradiance_cube() const -> const texture& { return *m_irradiance_cube; }
        [[nodiscard]] inline auto prefiltered_cube() const -> const texture& { return *m_prefiltered_cube; }
        [[nodiscard]] inline auto brdf_lut() const -> const texture& { return *m_brdf_lut; }

    private:
        auto generate_irradiance_cube() -> void;
        auto generate_prefilter_cube() -> void;
        auto generate_brdf_lookup_table() -> void;

        const mesh m_cube_mesh;
        std::shared_ptr<shader_cache> m_shader_cache {};
        eastl::optional<texture> m_environ_cube {};
        eastl::optional<texture> m_irradiance_cube {};
        eastl::optional<texture> m_prefiltered_cube {};
        eastl::optional<texture> m_brdf_lut {};
    };
}
