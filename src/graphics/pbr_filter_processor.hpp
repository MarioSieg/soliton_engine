// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"
#include "vulkancore/command_buffer.hpp"

#include "texture.hpp"

namespace lu::graphics {
    class pbr_filter_processor final : public no_copy, public no_move {
    public:
        static constexpr vk::Format k_brfd_lut_format = vk::Format::eR16G16Sfloat;

        pbr_filter_processor();

        [[nodiscard]] inline auto brdf_lut() const -> const texture& { return *m_brdf_lut; }

    private:
        auto generate_brdf_lookup_table() -> void;

        eastl::optional<texture> m_brdf_lut {};
    };
}
