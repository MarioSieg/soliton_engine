// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "swapchain.hpp"

#include <GLFW/glfw3native.h>

#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif

namespace vkb {
    swapchain::swapchain(vk::Instance instance, vk::PhysicalDevice physical_device, vk::Device logical_device) {
        passert(instance);
        passert(physical_device);
        passert(logical_device);
        m_instance = instance;
        m_physical_device = physical_device;
        m_logical_device = logical_device;
    }

    swapchain::~swapchain() {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    }

    auto swapchain::init_surface(GLFWwindow* window) -> void {
        log_info("Initializing swapchain surface");
        passert(window != nullptr);

        vkccheck(glfwCreateWindowSurface(m_instance, window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&m_surface)));

        // Get available queue family properties
        std::uint32_t num_qeueues = 0;
        m_physical_device.getQueueFamilyProperties(&num_qeueues, nullptr);
        std::vector<vk::QueueFamilyProperties> families {};
        families.resize(num_qeueues);
        m_physical_device.getQueueFamilyProperties(&num_qeueues, families.data());

        // Iterate over each queue to learn whether it supports presenting:
        // Find a queue with present support
        // Will be used to present the swap chain images to the windowing system
        std::vector<vk::Bool32> supports_present {};
        supports_present.resize(num_qeueues);
        for (std::uint32_t i = 0; i < num_qeueues; ++i) {
            supports_present[i] = m_physical_device.getSurfaceSupportKHR(i, m_surface);
        }

        // Search for a graphics and a present queue in the array of queue
        // families, try to find one that supports both
        std::uint32_t graphics_queue_node_index = std::numeric_limits<std::uint32_t>::max();
        std::uint32_t present_queue_node_index = std::numeric_limits<std::uint32_t>::max();
        for (std::uint32_t i = 0; i < num_qeueues; i++)  {
            if (families[i].queueFlags & vk::QueueFlagBits::eGraphics)  {
                if (graphics_queue_node_index == std::numeric_limits<std::uint32_t>::max())  {
                    graphics_queue_node_index = i;
                }
                if (supports_present[i] == vk::True)  {
                    graphics_queue_node_index = i;
                    present_queue_node_index = i;
                    break;
                }
            }
        }
        if (present_queue_node_index == std::numeric_limits<std::uint32_t>::max()) {
            // If there's no queue that supports both present and graphics
            // try to find a separate present queue
            for (std::uint32_t i = 0; i < num_qeueues; ++i)  {
                if (supports_present[i] == vk::True)  {
                    present_queue_node_index = i;
                    break;
                }
            }
        }

        passert(graphics_queue_node_index != std::numeric_limits<std::uint32_t>::max()
            && present_queue_node_index != std::numeric_limits<std::uint32_t>::max()); // Exit if either a graphics or a presenting queue hasn't been found
        passert(graphics_queue_node_index == present_queue_node_index); // Separate graphics and presenting queues are not supported yet

        m_queue_node_index = graphics_queue_node_index;

        std::uint32_t num_formats = 0;
        vkcheck(m_physical_device.getSurfaceFormatsKHR(m_surface, &num_formats, nullptr));
        std::vector<vk::SurfaceFormatKHR> formats {};
        formats.resize(num_formats);
        vkcheck(m_physical_device.getSurfaceFormatsKHR(m_surface, &num_formats, formats.data()));

        // We want to get a format that best suits our needs, so we try to get one from a set of preferred formats
        // Initialize the format to the first one returned by the implementation in case we can't find one of the preffered formats
        vk::SurfaceFormatKHR selected_format = formats[0];
        std::vector<vk::Format> preferred_image_formats = {
            vk::Format::eB8G8R8A8Unorm,
            vk::Format::eR8G8B8A8Unorm,
            vk::Format::eA8B8G8R8UnormPack32
        };

        for (const vk::SurfaceFormatKHR& available : formats) {
            if (std::ranges::find(preferred_image_formats, available.format) != preferred_image_formats.end()) {
                selected_format = available;
                break;
            }
        }

        m_format = selected_format.format;
        m_color_space = selected_format.colorSpace;

        log_info("Swapchain surface initialized: {}", string_VkFormat(static_cast<VkFormat>(m_format)));
    }
}
