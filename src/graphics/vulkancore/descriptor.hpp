// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "prelude.hpp"

#include <ankerl/unordered_dense.h>

namespace lu::graphics {
    class descriptor_allocator final : public no_copy, public no_move {
    public:
        descriptor_allocator() = default;
        ~descriptor_allocator();

        auto reset_all_pools() -> void;
        [[nodiscard]] auto allocate(vk::DescriptorSet& set, vk::DescriptorSetLayout layout) -> bool;

    private:
        using pool_sizes = std::vector<std::pair<vk::DescriptorType, float>>;
        pool_sizes m_pool_sizes {
            { vk::DescriptorType::eSampler, .5f },
            { vk::DescriptorType::eCombinedImageSampler, 4.f },
            { vk::DescriptorType::eSampledImage, 4.f },
            { vk::DescriptorType::eStorageImage, 1.f },
            { vk::DescriptorType::eUniformTexelBuffer, 1.f },
            { vk::DescriptorType::eStorageTexelBuffer, 1.f },
            { vk::DescriptorType::eUniformBuffer, 2.f },
            { vk::DescriptorType::eStorageBuffer, 2.f },
            { vk::DescriptorType::eUniformBufferDynamic, 1.f },
            { vk::DescriptorType::eStorageBufferDynamic, 1.f },
            { vk::DescriptorType::eInputAttachment, .5f },
            { vk::DescriptorType::eAccelerationStructureKHR, .5f },
        };

        vk::DescriptorPool m_current_pool {};
        std::vector<vk::DescriptorPool> m_used_pools {};
        std::vector<vk::DescriptorPool> m_free_pools {};

        [[nodiscard]] auto request_pool() -> vk::DescriptorPool;
        [[nodiscard]] static auto create_pool(const pool_sizes& sizes, std::int32_t count, vk::DescriptorPoolCreateFlagBits flags) -> vk::DescriptorPool;
    };

    class descriptor_layout_cache final {
    public:
        descriptor_layout_cache() = default;
        ~descriptor_layout_cache();

        [[nodiscard]] auto create_layout(vk::DescriptorSetLayoutCreateInfo& info) -> vk::DescriptorSetLayout;

        struct descriptor_layout_info final {
            std::vector<vk::DescriptorSetLayoutBinding> bindings {};
            auto operator == (const descriptor_layout_info& other) const -> bool;
            auto operator != (const descriptor_layout_info& other) const -> bool;
            [[nodiscard]] auto get_hash() const noexcept -> std::size_t;
        };

    private:
        struct descriptor_layout_hash final {
            auto operator()(const descriptor_layout_info& info) const noexcept -> std::size_t { return info.get_hash(); }
        };
        using layout_cache = ankerl::unordered_dense::map<descriptor_layout_info, vk::DescriptorSetLayout, descriptor_layout_hash>;
        layout_cache m_layout_cache {};
    };
}
