// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "descriptor.hpp"
#include "context.hpp"

namespace lu::vkb {
    auto descriptor_allocator::reset_all_pools() -> void {
        for (auto&& pool : m_used_pools) {
            vkb::vkdvc().resetDescriptorPool(pool, vk::DescriptorPoolResetFlags{});
        }
        m_free_pools = std::move(m_used_pools);
        m_used_pools = {};
        m_current_pool = VK_NULL_HANDLE;
    }

    auto descriptor_allocator::allocate(vk::DescriptorSet& set, vk::DescriptorSetLayout layout) -> bool {
        // when current working pool is null then request new
        if (m_current_pool == VK_NULL_HANDLE) {
            m_current_pool = request_pool();
            m_used_pools.emplace_back(m_current_pool);
        }

        vk::DescriptorSetAllocateInfo alloc_info {};
        alloc_info.descriptorPool = m_current_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &layout;

        vk::Result result = vkb::vkdvc().allocateDescriptorSets(&alloc_info, &set);
        bool reallocation = false;
        switch (result) {
            case vk::Result::eSuccess: return true;
            case vk::Result::eErrorFragmentedPool:
            case vk::Result::eErrorOutOfPoolMemory: reallocation = true; break;
            default: vkcheck(result); break;
        }

        if (reallocation) {
            m_current_pool = request_pool();
            m_used_pools.emplace_back(m_current_pool);
            alloc_info.descriptorPool = m_current_pool;
            result = vkb::vkdvc().allocateDescriptorSets(&alloc_info, &set);
            if (result == vk::Result::eSuccess) [[likely]] {
                return true;
            }
        }

        vkcheck(result);
        return false;
    }

    descriptor_allocator::~descriptor_allocator() {
        reset_all_pools();
        for (auto&& pool : m_free_pools) {
            vkb::vkdvc().destroyDescriptorPool(pool, vkb::get_alloc());
        }
        for (auto&& pool : m_used_pools) {
            vkb::vkdvc().destroyDescriptorPool(pool, vkb::get_alloc());
        }
    }

    auto descriptor_allocator::request_pool(const std::uint32_t count) -> vk::DescriptorPool {
        if (!m_free_pools.empty()) {
            auto pool = m_free_pools.back();
            m_free_pools.pop_back();
            return pool;
        } else {
            return create_pool(configured_pool_sizes, count ? count : alloc_granularity, {});
        }
    }

    auto descriptor_allocator::create_pool(const pool_sizes& sizes, const std::uint32_t alloc_granularity, const vk::DescriptorPoolCreateFlagBits flags) -> vk::DescriptorPool {
        eastl::vector<vk::DescriptorPoolSize> pool_sizes {};
        pool_sizes.reserve(sizes.size());
        for (auto&& [type, num] : sizes)
            pool_sizes.emplace_back(vk::DescriptorPoolSize{.type=type, .descriptorCount=static_cast<std::uint32_t>(num * static_cast<float>(alloc_granularity))});
        vk::DescriptorPoolCreateInfo pool_info {};
        pool_info.flags = flags;
        pool_info.maxSets = alloc_granularity;
        pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();
        vk::DescriptorPool pool {};
        vkcheck(vkb::vkdvc().createDescriptorPool(&pool_info, vkb::get_alloc(), &pool));
        return pool;
    }

    auto descriptor_layout_cache::descriptor_layout_info::operator == (const descriptor_layout_cache::descriptor_layout_info& other) const -> bool {
        return other.get_hash() == get_hash();
    }

    auto descriptor_layout_cache::descriptor_layout_info::operator != (const descriptor_layout_cache::descriptor_layout_info& other) const -> bool {
        return !(*this == other);
    }

    auto descriptor_layout_cache::descriptor_layout_info::get_hash() const noexcept -> std::size_t {
        return crc32(reinterpret_cast<const std::byte*>(bindings.data()), bindings.size() * sizeof(bindings[0]));
    }

    descriptor_layout_cache::~descriptor_layout_cache() {
        for (auto&& [_, layout] : m_layout_cache) {
            //vkb::vkdvc().destroyDescriptorSetLayout(layout, vkb::get_alloc()); TODO
        }
        m_layout_cache.clear();
    }

    auto descriptor_layout_cache::create_layout(vk::DescriptorSetLayoutCreateInfo& info) -> vk::DescriptorSetLayout {
        descriptor_layout_info layout_info {};
        layout_info.bindings.reserve(info.bindingCount);
        bool is_sorted = true;
        std::int32_t last_binding = -1;
        for (std::uint32_t i = 0; i < info.bindingCount; ++i) {
            layout_info.bindings.emplace_back(info.pBindings[i]);
            if (static_cast<decltype(last_binding)>(info.pBindings[i].binding) > last_binding) {
                last_binding = static_cast<decltype(last_binding)>(info.pBindings[i].binding);
            } else {
                is_sorted = false;
            }
        }
        if (!is_sorted) {
            eastl::sort(layout_info.bindings.begin(), layout_info.bindings.end(), [](const vk::DescriptorSetLayoutBinding& a, const vk::DescriptorSetLayoutBinding& b) {
                return a.binding < b.binding;
            });
        }
        auto it = m_layout_cache.find(layout_info);
        if (it != m_layout_cache.end()) {
            return it->second;
        } else {
            vk::DescriptorSetLayout layout {};
            vkcheck(vkb::vkdvc().createDescriptorSetLayout(&info, vkb::get_alloc(), &layout));
            m_layout_cache[layout_info] = layout;
            return layout;
        }
    }

    auto descriptor_factory::build_no_info(vk::DescriptorSetLayout& layout, vk::DescriptorSet& set) -> void {
        vk::DescriptorSetLayoutCreateInfo layout_info {};
        layout_info.bindingCount = static_cast<std::uint32_t>(m_bindings.size());
        layout_info.pBindings = m_bindings.data();
        layout = m_cache->create_layout(layout_info);
        panic_assert(m_allocator->allocate(set, layout));
    }

    auto descriptor_factory::build_no_info(vk::DescriptorSet& set) -> void {
        vk::DescriptorSetLayout layout {};
        build_no_info(layout, set);
    }

    auto descriptor_factory::build_no_info_push(vk::DescriptorSetLayout& layout) -> void {
        vk::DescriptorSetLayoutCreateInfo layout_info {};
        layout_info.bindingCount = static_cast<std::uint32_t>(m_bindings.size());
        layout_info.pBindings = m_bindings.data();
        layout = m_cache->create_layout(layout_info);
    }

    auto descriptor_factory::build(vk::DescriptorSet& set, vk::DescriptorSetLayout& layout) -> bool {
        vk::DescriptorSet ret_set {};
        vk::DescriptorSetLayout ret_layout {};
        vk::DescriptorSetLayoutCreateInfo layout_info {};
        layout_info.bindingCount = static_cast<std::uint32_t>(m_bindings.size());
        layout_info.pBindings = m_bindings.data();
        ret_layout = m_cache->create_layout(layout_info);
        if (!m_allocator->allocate(ret_set, ret_layout)) [[unlikely]] {
            return false;
        }
        eastl::fixed_vector<vk::WriteDescriptorSet, 16> writes {};
        writes.reserve(m_write_descriptors.size());
        for (auto&& dc : m_write_descriptors) {
            vk::WriteDescriptorSet write {};
            write.dstSet = ret_set;
            write.dstBinding = dc.binding;
            write.descriptorCount = dc.count;
            write.descriptorType = dc.type;
            if (dc.is_image) {
                write.pImageInfo = dc.image_info;
            } else {
                write.pBufferInfo = dc.buffer_info;
            }
            writes.emplace_back(write);
        }
        if (!writes.empty())
            vkb::vkdvc().updateDescriptorSets(static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);
        set = ret_set;
        layout = ret_layout;
        return true;
    }

    auto descriptor_factory::build(vk::DescriptorSet& set) -> bool {
        vk::DescriptorSetLayout layout {};
        return build(set, layout);
    }

    auto descriptor_factory::bind_no_info_stage(const vk::DescriptorType type, const vk::ShaderStageFlagBits stage_flags, const std::uint32_t binding, const std::uint32_t count) -> descriptor_factory& {
        vk::DescriptorSetLayoutBinding binding_info {};
        binding_info.binding = binding;
        binding_info.descriptorCount = count;
        binding_info.descriptorType = type;
        binding_info.stageFlags = stage_flags;
        m_bindings.emplace_back(binding_info);
        return *this;
    }

    auto descriptor_factory::bind_no_info_stage(const vk::DescriptorType type, const std::uint32_t binding, const std::uint32_t count) -> descriptor_factory& {
        return bind_no_info_stage(type, static_cast<vk::ShaderStageFlagBits>(static_cast<std::underlying_type_t<vk::ShaderStageFlagBits>>(k_common_stages)), binding, count);
    }

    auto descriptor_factory::bind_buffers(
        const std::uint32_t binding,
        const std::uint32_t count,
        vk::DescriptorBufferInfo* const buffer_info,
        const vk::DescriptorType type,
        const vk::ShaderStageFlagBits flags
    ) -> descriptor_factory& {

        vk::DescriptorSetLayoutBinding binding_info {};
        binding_info.binding = binding;
        binding_info.descriptorCount = count;
        binding_info.descriptorType = type;
        binding_info.stageFlags = flags;
        m_bindings.emplace_back(binding_info);

        descriptor_write_container dc {};
        dc.buffer_info = buffer_info;
        dc.binding = binding;
        dc.count = count;
        dc.type = type;
        dc.is_image = false;
        m_write_descriptors.emplace_back(dc);

        return *this;
    }

    auto descriptor_factory::bind_images(
        const std::uint32_t binding,
        const std::uint32_t count,
        vk::DescriptorImageInfo* const buffer_info,
        const vk::DescriptorType type,
        const vk::ShaderStageFlagBits flags
    ) -> descriptor_factory& {

        vk::DescriptorSetLayoutBinding binding_info {};
        binding_info.binding = binding;
        binding_info.descriptorCount = count;
        binding_info.descriptorType = type;
        binding_info.stageFlags = flags;
        m_bindings.emplace_back(binding_info);

        descriptor_write_container dc {};
        dc.image_info = buffer_info;
        dc.binding = binding;
        dc.count = count;
        dc.type = type;
        dc.is_image = true;
        m_write_descriptors.emplace_back(dc);

        return *this;
    }
}
