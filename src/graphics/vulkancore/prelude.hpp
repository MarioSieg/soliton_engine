// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../../core/core.hpp"

// Include GLSL/C++ shader header
#include "../../../../engine_assets/shaders/include/cpp_shared_structures.h"

// Keep Vulkan's C++ source code clean and remove some magic
// Useful when tutorials or other stuff use plain C, so its easier to compare
#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_STRUCT_SETTERS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vk_enum_string_helper.h>
#if USE_MIMALLOC
#include <mimalloc.h>
#endif

#include <vk_mem_alloc.h>

#if PLATFORM_OSX
#   include <vulkan/vulkan_beta.h>
#endif

namespace lu::vkb {
    extern auto dump_physical_device_props(const vk::PhysicalDeviceProperties& props) -> void;

#if USE_MIMALLOC
    constexpr vk::AllocationCallbacks s_vk_alloc = [] {
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
#endif

    [[nodiscard]] consteval auto get_alloc() noexcept -> const vk::AllocationCallbacks* {
#if USE_MIMALLOC
        return &s_vk_alloc;
#else
        return nullptr;
#endif
    }
}

#define vkcheck(f) \
    if (const vk::Result rrr = (f); rrr != vk::Result::eSuccess) [[unlikely]] { \
        log_error("Vulkan error: {} -> " #f, string_VkResult(static_cast<VkResult>(rrr))); \
        panic("Vulkan error: {} -> " #f, string_VkResult(static_cast<VkResult>(rrr))); \
    }

#define vkccheck(f) \
    if (const VkResult rrr = (f); rrr != VK_SUCCESS) [[unlikely]] { \
        log_error("Vulkan error: {} -> " #f, string_VkResult(rrr)); \
        panic("Vulkan error: {} -> " #f, string_VkResult(rrr)); \
    }
