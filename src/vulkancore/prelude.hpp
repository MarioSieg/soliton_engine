// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_enum_string_helper.h>

#include "../core/core.hpp"

namespace vkb {
    extern auto dump_physical_device_props(const vk::PhysicalDeviceProperties& props) -> void;
}

#define vkcheck(f) \
    if (vk::Result result = (f); result != vk::Result::eSuccess) [[unlikely]] { \
        log_error("Vulkan error: {}", string_VkResult(static_cast<VkResult>(result))); \
        passert(result == vk::Result::eSuccess); \
    }

#define vkccheck(f) \
    if (VkResult result = (f); result != VK_SUCCESS) [[unlikely]] { \
        log_error("Vulkan error: {}", string_VkResult(result)); \
        passert(result == VK_SUCCESS); \
    }

