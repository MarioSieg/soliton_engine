// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

// Keep Vulkan's C++ source code clean and remove some magic
// Useful when tutorials or other stuff use plain C, so its easier to compare
#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_STRUCT_SETTERS
#include <vulkan/vulkan.hpp>
#include <vulkan/vk_enum_string_helper.h>
#include <mimalloc.h>

#include "../core/core.hpp"

namespace vkb {
    extern auto dump_physical_device_props(const vk::PhysicalDeviceProperties& props) -> void;

    constexpr vk::AllocationCallbacks s_allocator = [] {
        vk::AllocationCallbacks allocator {};
        allocator.pfnAllocation = +[]([[maybe_unused]] void* usr, const std::size_t size, const std::size_t alignment, [[maybe_unused]] VkSystemAllocationScope scope) -> void* {
            return mi_malloc_aligned(size, alignment);
        };
        allocator.pfnReallocation = +[]([[maybe_unused]] void* usr, void* block, const std::size_t size, std::size_t alignment, [[maybe_unused]] VkSystemAllocationScope scope) -> void* {
            return mi_realloc_aligned(block, size, alignment);
        };
        allocator.pfnFree = +[]([[maybe_unused]] void* usr, void* mem) -> void {
            mi_free(mem);
        };
        return allocator;
    }();
}

#define vkcheck(f) \
    if (const vk::Result rrr = (f); rrr != vk::Result::eSuccess) [[unlikely]] { \
        log_error("Vulkan error: {}", string_VkResult(static_cast<VkResult>(rrr))); \
        passert(rrr == vk::Result::eSuccess); \
    }

#define vkccheck(f) \
    if (const VkResult rrr = (f); rrr != VK_SUCCESS) [[unlikely]] { \
        log_error("Vulkan error: {}", string_VkResult(rrr)); \
        passert(rrr == VK_SUCCESS); \
    }
