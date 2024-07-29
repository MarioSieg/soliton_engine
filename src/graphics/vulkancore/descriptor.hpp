// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

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

    class descriptor_layout_cache final : public no_copy, public no_move {
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

    class descriptor_factory final : public no_copy, public no_move {
    public:
        static constexpr vk::Flags<vk::ShaderStageFlagBits> k_common_stages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute;

        inline descriptor_factory(descriptor_layout_cache& cache, descriptor_allocator& allocator) noexcept : m_cache{cache}, m_allocator{allocator} {};
        ~descriptor_factory() = default;

        [[nodiscard]] auto build(vk::DescriptorSet& set, vk::DescriptorSetLayout& layout) -> bool;
        [[nodiscard]] auto build(vk::DescriptorSet& set) -> bool;
        auto build_no_info(vk::DescriptorSetLayout& layout, vk::DescriptorSet& set) -> void;
        auto build_no_info(vk::DescriptorSet& set) -> void;
        auto build_no_info_push(vk::DescriptorSetLayout& layout) -> void;
        auto bind_no_info_stage(vk::DescriptorType type, vk::ShaderStageFlagBits stage_flags, std::uint32_t binding, std::uint32_t count = 1) -> descriptor_factory&;
        auto bind_no_info_stage(vk::DescriptorType type, std::uint32_t binding, std::uint32_t count = 1) -> descriptor_factory&;
        auto bind_buffers(
            std::uint32_t bindings,
            std::uint32_t count,
            vk::DescriptorBufferInfo* buffer_info,
            vk::DescriptorType type,
            vk::ShaderStageFlagBits flags
        ) -> descriptor_factory&;
        auto bind_images(
            std::uint32_t bindings,
            std::uint32_t count,
            vk::DescriptorImageInfo* buffer_info,
            vk::DescriptorType type,
            vk::ShaderStageFlagBits flags
        ) -> descriptor_factory&;

    private:
        struct descriptor_write_container final {
            vk::DescriptorImageInfo* image_info {};
            vk::DescriptorBufferInfo* buffer_info {};
            std::uint32_t binding {};
            vk::DescriptorType type {};
            std::uint32_t count {};
            bool is_image {};
        };

        std::vector<descriptor_write_container> m_write_descriptors {};
        std::vector<vk::DescriptorSetLayoutBinding> m_bindings {};
        descriptor_layout_cache& m_cache;
        descriptor_allocator& m_allocator;
    };
}
