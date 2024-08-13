// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"
#include "vulkancore/buffer.hpp"

namespace lu::graphics {
    class temporal_blue_noise final : public no_copy, public no_move {
    public:
        temporal_blue_noise();

        struct buffer_bundle final {
            eastl::optional<vkb::buffer> sobol_buffer;
            eastl::optional<vkb::buffer> ranking_tile_buffer;
            eastl::optional<vkb::buffer> scrambling_tile_buffer;

            vk::DescriptorSet set {};
            vk::DescriptorSetLayout layout {};

            auto build_set() -> void;
        };

        buffer_bundle spp_1_buffer {};
        buffer_bundle spp_2_buffer {};
        buffer_bundle spp_4_buffer {};
        buffer_bundle spp_8_buffer {};
        buffer_bundle spp_16_buffer {};
        buffer_bundle spp_32_buffer {};
        buffer_bundle spp_64_buffer {};
        buffer_bundle spp_128_buffer {};
        buffer_bundle spp_256_buffer {};
    };
}
