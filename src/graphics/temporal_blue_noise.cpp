// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "temporal_blue_noise.hpp"
#include "vulkancore/context.hpp"

namespace soliton::graphics {
    namespace blue_noise_256_spp {
        #include "blue_noise_samplers/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_256spp.inl"
    }
    namespace blue_noise_128_spp {
        #include "blue_noise_samplers/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_128spp.inl"
    }
    namespace blue_noise_64_spp {
        #include "blue_noise_samplers/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_64spp.inl"
    }
    namespace blue_noise_32_spp {
        #include "blue_noise_samplers/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_32spp.inl"
    }
    namespace blue_noise_16_spp {
        #include "blue_noise_samplers/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_16spp.inl"
    }
    namespace blue_noise_8_spp {
        #include "blue_noise_samplers/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_8spp.inl"
    }
    namespace blue_noise_4_spp {
        #include "blue_noise_samplers/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_4spp.inl"
    }
    namespace blue_noise_2_spp {
        #include "blue_noise_samplers/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_2spp.inl"
    }
    namespace blue_noise_1_spp {
        #include "blue_noise_samplers/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp.inl"
    }

    auto temporal_blue_noise::buffer_bundle::build_set() -> void {
        vkb::descriptor_factory factory {vkb::ctx().descriptor_factory_begin()};

        eastl::array<vk::DescriptorBufferInfo, 3> infos {
            vk::DescriptorBufferInfo {
                .buffer = sobol_buffer->get_buffer(),
                .offset = 0,
                .range = sobol_buffer->get_size()
            },
            vk::DescriptorBufferInfo {
                .buffer = ranking_tile_buffer->get_buffer(),
                .offset = 0,
                .range = ranking_tile_buffer->get_size()
            },
            vk::DescriptorBufferInfo {
                .buffer = scrambling_tile_buffer->get_buffer(),
                .offset = 0,
                .range = scrambling_tile_buffer->get_size()
            }
        };
        for (std::uint32_t i = 0; auto&& info : infos) {
            factory.bind_buffers(
                i++,
                1,
                &info,
                vk::DescriptorType::eStorageBuffer,
                vk::ShaderStageFlagBits::eFragment
            );
        }
        panic_assert(factory.build(set, layout));
    }

    temporal_blue_noise::temporal_blue_noise() {
        constexpr auto build_buffer = [](eastl::optional<vkb::buffer>& buf, const void* const ptr, const std::size_t size) {
            buf.emplace(
                size,
                0,
                vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
                VMA_MEMORY_USAGE_GPU_ONLY,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                ptr
            );
        };

        #define build_noise_buffer(target, workspace) \
            build_buffer(target.sobol_buffer, workspace::sobol_256spp_256d, sizeof(workspace::sobol_256spp_256d)); \
            build_buffer(target.ranking_tile_buffer, workspace::rankingTile, sizeof(workspace::rankingTile)); \
            build_buffer(target.scrambling_tile_buffer, workspace::scramblingTile, sizeof(workspace::scramblingTile)); \
            target.build_set();

        build_noise_buffer(spp_1_buffer, blue_noise_1_spp);
        build_noise_buffer(spp_2_buffer, blue_noise_2_spp);
        build_noise_buffer(spp_4_buffer, blue_noise_4_spp);
        build_noise_buffer(spp_8_buffer, blue_noise_8_spp);
        build_noise_buffer(spp_16_buffer, blue_noise_16_spp);
        build_noise_buffer(spp_32_buffer, blue_noise_32_spp);
        build_noise_buffer(spp_64_buffer, blue_noise_64_spp);
        build_noise_buffer(spp_128_buffer, blue_noise_128_spp);
        build_noise_buffer(spp_256_buffer, blue_noise_256_spp);

        #undef build_noise_buffer
    }
}
