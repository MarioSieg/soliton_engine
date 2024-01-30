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
        for (std::uint32_t i = 0; i < m_image_count; ++i) {
            m_logical_device.destroyImageView(m_buffers[i].view, &s_allocator);
        }
        m_logical_device.destroySwapchainKHR(m_swapchain, &s_allocator);
        m_instance.destroySurfaceKHR(m_surface, &s_allocator);
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
            vkcheck(m_physical_device.getSurfaceSupportKHR(i, m_surface, &supports_present[i]));
        }

        // Search for a graphics and a present queue in the array of queue
        // families, try to find one that supports both
        std::uint32_t graphics_queue_node_index = std::numeric_limits<std::uint32_t>::max();
        std::uint32_t present_queue_node_index = std::numeric_limits<std::uint32_t>::max();
        for (std::uint32_t i = 0; i < num_qeueues; ++i)  {
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

    auto swapchain::create(std::uint32_t& w, std::uint32_t& h, bool vsync, bool fullscreen) -> void {
        log_info("Creating swapchain: {}x{}, VSync: {}, Fullscreen: {}", w, h, vsync, fullscreen);
        vk::SwapchainKHR old_swapchain = m_swapchain;
        vk::SurfaceCapabilitiesKHR surface_capabilities;
        vkcheck(m_physical_device.getSurfaceCapabilitiesKHR(m_surface, &surface_capabilities));
        std::uint32_t num_present_modes = 0;
        vkcheck(m_physical_device.getSurfacePresentModesKHR(m_surface, &num_present_modes, nullptr));
        passert(num_present_modes != 0);
        std::vector<vk::PresentModeKHR> present_modes {};
        present_modes.resize(num_present_modes);
        vkcheck(m_physical_device.getSurfacePresentModesKHR(m_surface, &num_present_modes, present_modes.data()));
        vk::Extent2D swapchain_extent {};
        // If width (and height) equals the special value 0xffffffff, the size of the surface will be set by the swapchain
        if (surface_capabilities.currentExtent.width == 0xffffffff) {
            // If the surface size is undefined, the size is set to
            // the size of the images requested.
            swapchain_extent.width = w;
            swapchain_extent.height = h;
        } else {
            // If the surface size is defined, the swap chain size must match
            swapchain_extent = surface_capabilities.currentExtent;
            w = surface_capabilities.currentExtent.width;
            h = surface_capabilities.currentExtent.height;
        }

        // Select a present mode for the swapchain

        // The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
        // This mode waits for the vertical blank ("v-sync")
        vk::PresentModeKHR swapchain_present_mode = vk::PresentModeKHR::eFifo;

        // If v-sync is not requested, try to find a mailbox mode
        // It's the lowest latency non-tearing present mode available
        if (!vsync) {
            for (std::uint32_t i = 0; i < num_present_modes; ++i) {
                if (present_modes[i] == vk::PresentModeKHR::eMailbox) {
                    swapchain_present_mode = vk::PresentModeKHR::eMailbox;
                    break;
                }
                if (present_modes[i] == vk::PresentModeKHR::eImmediate) {
                    swapchain_present_mode = vk::PresentModeKHR::eImmediate;
                }
            }
        }

        std::uint32_t desired_swapchain_images = surface_capabilities.minImageCount + 1;
        if (surface_capabilities.maxImageCount > 0 && desired_swapchain_images > surface_capabilities.maxImageCount) {
            desired_swapchain_images = surface_capabilities.maxImageCount;
        }

        vk::SurfaceTransformFlagBitsKHR pre_transform;
        if (surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
            pre_transform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        } else {
            pre_transform = surface_capabilities.currentTransform;
        }

        // Find a supported composite alpha format (not all devices support alpha opaque)
        vk::CompositeAlphaFlagBitsKHR composite_alpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        // Simply select the first composite alpha format available
        static constexpr std::array<vk::CompositeAlphaFlagBitsKHR, 4> composite_alpha_flags = {
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
            vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
            vk::CompositeAlphaFlagBitsKHR::eInherit
        };
        for (const vk::CompositeAlphaFlagBitsKHR& composite_alpha_flag : composite_alpha_flags) {
            if (surface_capabilities.supportedCompositeAlpha & composite_alpha_flag) {
                composite_alpha = composite_alpha_flag;
                break;
            }
        }

        vk::SwapchainCreateInfoKHR swapchain_ci {};
        swapchain_ci.surface = m_surface;
        swapchain_ci.minImageCount = desired_swapchain_images;
        swapchain_ci.imageFormat = m_format;
        swapchain_ci.imageColorSpace = m_color_space;
        swapchain_ci.imageExtent = vk::Extent2D { swapchain_extent.width, swapchain_extent.height };
        swapchain_ci.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        swapchain_ci.preTransform = pre_transform;
        swapchain_ci.imageArrayLayers = 1;
        swapchain_ci.imageSharingMode = vk::SharingMode::eExclusive;
        swapchain_ci.queueFamilyIndexCount = 0;
        swapchain_ci.pQueueFamilyIndices = nullptr;
        swapchain_ci.presentMode = swapchain_present_mode;
        swapchain_ci.oldSwapchain = old_swapchain; //Setting oldSwapChain to the saved handle of the previous swapchain aids in resource reuse and makes sure that we can still present already acquired images
        swapchain_ci.clipped = vk::True; // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
        swapchain_ci.compositeAlpha = composite_alpha;

        // Enable transfer source on swap chain images if supported
        if (surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferSrc) {
            swapchain_ci.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc;
        }

        // Enable transfer destination on swap chain images if supported
        if (surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst) {
            swapchain_ci.imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
        }

        vkcheck(m_logical_device.createSwapchainKHR(&swapchain_ci, &s_allocator, &m_swapchain));

        // If an existing swap chain is re-created, destroy the old swap chain
        // This also cleans up all the presentable images
        if (old_swapchain) {
            for (std::uint32_t i = 0; i < m_image_count; ++i) {
                m_logical_device.destroyImageView(m_buffers[i].view, &s_allocator);
            }
            m_logical_device.destroySwapchainKHR(old_swapchain, &s_allocator);
        }

        // Get the swap chain images
        vkcheck(m_logical_device.getSwapchainImagesKHR(m_swapchain, &m_image_count, nullptr));
        m_images.resize(m_image_count);
        vkcheck(m_logical_device.getSwapchainImagesKHR(m_swapchain, &m_image_count, m_images.data()));

        // Get the swap chain buffers containing the image and imageview
        m_buffers.resize(m_image_count);
        for (std::uint32_t i = 0; i < m_image_count; ++i) {
            vk::ImageViewCreateInfo color_image_view {};
            color_image_view.format = m_format;
            color_image_view.components = {
                vk::ComponentSwizzle::eR,
                vk::ComponentSwizzle::eG,
                vk::ComponentSwizzle::eB,
                vk::ComponentSwizzle::eA
            };
            color_image_view.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            color_image_view.subresourceRange.baseMipLevel = 0;
            color_image_view.subresourceRange.levelCount = 1;
            color_image_view.subresourceRange.baseArrayLayer = 0;
            color_image_view.subresourceRange.layerCount = 1;
            color_image_view.viewType = vk::ImageViewType::e2D;
            color_image_view.flags = {};
            m_buffers[i].image = m_images[i];
            color_image_view.image = m_images[i];
            vkcheck(m_logical_device.createImageView(&color_image_view, &s_allocator, &m_buffers[i].view));
        }
    }

    auto swapchain::acquire_next_image(const vk::Semaphore present_complete_semaphore, std::uint32_t& idx) const -> vk::Result {
        return m_logical_device.acquireNextImageKHR(
            m_swapchain,
            std::numeric_limits<std::uint64_t>::max(),
            present_complete_semaphore,
            nullptr,
            &idx
        );
    }

    auto swapchain::queue_present(
        const vk::Queue queue,
        const std::uint32_t image_index,
        const vk::Semaphore wait_semaphore
    ) const -> vk::Result {
        vk::PresentInfoKHR present_info {};
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &m_swapchain;
        present_info.pImageIndices = &image_index;
        if (wait_semaphore) {
            present_info.pWaitSemaphores = &wait_semaphore;
            present_info.waitSemaphoreCount = 1;
        }
        return queue.presentKHR(&present_info);
    }
}
