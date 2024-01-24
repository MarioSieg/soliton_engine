// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <vulkan/vulkan.hpp>

#include "../core/core.hpp"

namespace vkb {
    [[nodiscard]] extern auto get_error_string(vk::Result result) noexcept -> const char*;
    [[nodiscard]] extern auto physical_device_type_string(vk::PhysicalDeviceType type) noexcept -> const char*;
    extern auto dump_physical_device_props(const vk::PhysicalDeviceProperties& props) -> void;
}

#define vkcheck(f) \
    if (vk::Result result = (f); result != vk::Result::eSuccess) [[unlikely]] { \
        log_error("Vulkan error: {}", vkb::get_error_string(result)); \
        passert(result == vk::Result::eSuccess); \
    }

#define vkccheck(f) \
    if (VkResult result = (f); result != VK_SUCCESS) [[unlikely]] { \
        log_error("Vulkan error: {}", vkb::get_error_string(static_cast<vk::Result>(result))); \
        passert(result == VK_SUCCESS); \
    }

