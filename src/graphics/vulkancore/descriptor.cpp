// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "descriptor.hpp"
#include "context.hpp"

namespace lu::graphics {
    auto descriptor_allocator::reset_all_pools() -> void {
        for (auto&& pool : m_used_pools) {
            vkb::vkdvc().resetDescriptorPool(pool, vk::DescriptorPoolResetFlags{});
        }
        m_free_pools = std::move(m_used_pools);
        m_used_pools = {};
        m_current_pool = VK_NULL_HANDLE;
    }

    auto descriptor_allocator::allocate(vk::DescriptorSet &set, vk::DescriptorSetLayout layout) -> bool {
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

    auto descriptor_allocator::request_pool() -> vk::DescriptorPool {
        if (!m_free_pools.empty()) {
            auto pool = m_free_pools.back();
            m_free_pools.pop_back();
            return pool;
        } else {
            return create_pool(m_pool_sizes, 1000, {});
        }
    }

    auto descriptor_allocator::create_pool(const pool_sizes& sizes, const std::int32_t count, const vk::DescriptorPoolCreateFlagBits flags) -> vk::DescriptorPool {
        std::vector<vk::DescriptorPoolSize> pool_sizes {};
        pool_sizes.reserve(sizes.size());
        for (auto&& [type, num] : sizes) {
            pool_sizes.emplace_back(vk::DescriptorPoolSize{.type=type, .descriptorCount=static_cast<std::uint32_t>(num * static_cast<float>(count))});
        }
        vk::DescriptorPoolCreateInfo pool_info {};
        pool_info.flags = flags;
        pool_info.maxSets = count;
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
        return crc32(reinterpret_cast<const std::byte*>(bindings.data()), bindings.size() * sizeof(std::decay_t<decltype(bindings[0])>));
    }

    descriptor_layout_cache::~descriptor_layout_cache() {
        for (auto&& [_, layout] : m_layout_cache) {
            vkb::vkdvc().destroyDescriptorSetLayout(layout, vkb::get_alloc());
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
            std::sort(layout_info.bindings.begin(), layout_info.bindings.end(), [](const vk::DescriptorSetLayoutBinding& a, const vk::DescriptorSetLayoutBinding& b) {
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
}
