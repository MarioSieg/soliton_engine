// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "device.hpp"

#include <GLFW/glfw3.h>

namespace soliton::vkb {
    // Beginning with the 1.3.216 Vulkan SDK, the Vulkan Loader is strictly enforcing the new VK_KHR_PORTABILITY_subset extension.
    static const system_variable<bool> sv_osx_use_vk_portability_enumeration {
        "renderer.osx_use_vk_portability_enumeration",
        {true}
    };

    device::device(
        const bool enable_validation,
        const bool require_stencil
    ) {
        log_info("Initializing Vulkan device...");

        m_enable_validation = enable_validation;
        m_api_version = k_vulkan_api_version;

        create_instance();
        find_physical_device();
        create_logical_device(k_enabled_features, k_device_extensions, nullptr);
        init_vma();
        panic_assert(find_supported_depth_format(require_stencil, m_depth_format));

        log_info("Vulkan device initialized");
    }

    device::~device() {
        vmaDestroyAllocator(m_allocator);
        m_logical_device.destroy(vkb::get_alloc());
        log_info("Destroying Vulkan device...");
        if (m_debug_utils_messenger != nullptr) {
            (*m_destroy_debug_utils_messenger_ext)(m_instance, m_debug_utils_messenger, reinterpret_cast<const VkAllocationCallbacks*>(vkb::get_alloc()));
            log_info("Destroyed Vulkan debug utils messenger");
        }
        m_instance.destroy(vkb::get_alloc());
        log_info("Destroyed Vulkan instance");
    }

    extern "C" VKAPI_ATTR auto VKAPI_CALL vulkan_debug_message_callback(
        const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* usr
    ) -> VkBool32 {
        if (!data->pMessageIdName || !data->pMessage) [[unlikely]] {
            return vk::False;
        }
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
            log_info("[{}] [{}] {}", data->messageIdNumber, data->pMessageIdName, data->pMessage);
        } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            log_info("[{}] [{}] {}", data->messageIdNumber, data->pMessageIdName, data->pMessage);
        } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            log_warn("[{}] [{}] {}", data->messageIdNumber, data->pMessageIdName, data->pMessage);
        } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            log_error("[{}] [{}] {}", data->messageIdNumber, data->pMessageIdName, data->pMessage);
        }
        return vk::False;
    }

    auto device::is_device_extension_supported(const char* extension) const -> bool {
        for (const vk::ExtensionProperties& ext : m_supported_device_extensions) {
            if (std::strcmp(ext.extensionName, extension) == 0) {
                return true;
            }
        }
        return false;
    }

    auto device::is_instance_extension_supported(const char* extension) const -> bool {
        return std::ranges::find(m_supported_instance_extensions, extension) != m_supported_instance_extensions.end();
    }

    auto device::get_mem_type_idx(
        std::uint32_t type_bits,
        vk::MemoryPropertyFlags properties,
        vk::Bool32& found
    ) const -> std::uint32_t {
        for (std::uint32_t i = 0; i < m_memory_properties.memoryTypeCount; i++) {
            if ((type_bits & 1) == 1) {
                if ((m_memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                    found = true;
                    return i;
                }
            }
            type_bits >>= 1;
        }
        found = false;
        return 0;
    }

    auto device::is_image_format_supported(
        const vk::ImageType type,
        const vk::Format format,
        const vk::ImageCreateFlags flags,
        const vk::ImageUsageFlags usage,
        const vk::ImageTiling tiling,
        bool& is_generally_supported,
        bool& is_mipgen_supported
    ) const -> void {
        is_mipgen_supported = false;

        vk::PhysicalDeviceImageFormatInfo2 info {};
        info.type = type;
        info.format = format;
        info.flags = flags;
        info.usage = usage;
        info.tiling = tiling;

        vk::ImageFormatProperties2 props {};
        is_generally_supported = m_physical_device.getImageFormatProperties2(&info, &props) == vk::Result::eSuccess;

        // check if format supports color writing for mipmap generation
        if (is_generally_supported) [[likely]] {
            vk::FormatProperties format_props {};
            m_physical_device.getFormatProperties(format, &format_props);
            is_mipgen_supported = static_cast<bool>(format_props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment);
        }
    }

    auto device::create_instance() -> void {
        log_info("Creating Vulkan instance...");

        const auto [major, minor] = unpack_version(k_lunam_engine_version);

        // Create application info
        vk::ApplicationInfo app_info {};
        app_info.pApplicationName = "Soliton Engine";
        app_info.applicationVersion = VK_MAKE_VERSION(major, minor, 0);
        app_info.pEngineName = "Soliton Engine";
        app_info.engineVersion = VK_MAKE_VERSION(major, minor, 0);
        app_info.apiVersion = m_api_version;

        ankerl::unordered_dense::set<eastl::string> requested_instance_extensions {
            VK_KHR_SURFACE_EXTENSION_NAME,
        };

        if (!glfwVulkanSupported()) [[unlikely]] {
            panic("GLFW does not support Vulkan");
        }

        std::uint32_t glfw_extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        if (!glfw_extension_count || glfw_extensions == nullptr) [[unlikely]] {
            panic("GLFW failed to retrieve required Vulkan instance extensions");
        }
        for (std::uint32_t i = 0; i < glfw_extension_count; ++i) {
            log_info("GLFW required instance extension: {}", glfw_extensions[i]);
            requested_instance_extensions.insert(glfw_extensions[i]);
        }

        // Enumerate supported instance extensions
        std::uint32_t ext_count = 0;
        vkcheck(vk::enumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr));
        if (ext_count) [[likely]] {
            eastl::vector<vk::ExtensionProperties> extensions {ext_count};
            if (vk::enumerateInstanceExtensionProperties(nullptr, &ext_count, extensions.data()) == vk::Result::eSuccess) {
                for (vk::ExtensionProperties& extension : extensions) {
                    m_supported_instance_extensions.emplace_back(extension.extensionName);
                    log_info("Supported instance extension: {}", extension.extensionName.data());
                }
            }
        }

        if constexpr (PLATFORM_OSX) {
            if (sv_osx_use_vk_portability_enumeration()) {
                requested_instance_extensions.insert(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            }
            requested_instance_extensions.insert(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }

        // Setup instance create info
        vk::InstanceCreateInfo instance_create_info {};
        instance_create_info.pNext = nullptr;
        instance_create_info.pApplicationInfo = &app_info;

        if constexpr (PLATFORM_OSX) { // macOS/iOS - enable MoltenVK portability extension
            if (sv_osx_use_vk_portability_enumeration()) {
                instance_create_info.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
            }
        }

        // Setup debug utils messenger create info
        if (m_enable_validation) {
            log_info("Enabling Vulkan validation layers...");
            if (std::ranges::find(m_supported_instance_extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != m_supported_instance_extensions.end()) {
                log_info("Enabling Vulkan debug utils extension...");
                m_debug_utils_messenger_ci.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
                m_debug_utils_messenger_ci.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
                m_debug_utils_messenger_ci.pfnUserCallback = &vulkan_debug_message_callback;
                m_debug_utils_messenger_ci.pNext = instance_create_info.pNext;
                instance_create_info.pNext = &m_debug_utils_messenger_ci;
                requested_instance_extensions.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            } else {
                log_warn("Vulkan debug utils extension not supported");
            }
        }

        // Setup enabled instance extensions
        eastl::vector<const char*> enabled_instance_extensions {};
        if (!requested_instance_extensions.empty()) [[likely]] {
            enabled_instance_extensions.reserve(requested_instance_extensions.size());
            for (const auto& ext : requested_instance_extensions) {
                log_info("Enabling instance extension: {}", ext);
                if (std::ranges::find(m_supported_instance_extensions, ext) == m_supported_instance_extensions.end()) [[unlikely]] {
                    panic("Instance extension {} not supported", ext);
                }
                enabled_instance_extensions.emplace_back(ext.c_str());
            }
            instance_create_info.enabledExtensionCount = static_cast<std::uint32_t>(enabled_instance_extensions.size());
            instance_create_info.ppEnabledExtensionNames = enabled_instance_extensions.data();
        }

        log_info("Fetching instance layers...");

        std::uint32_t layer_count = 0;
        vkcheck(vk::enumerateInstanceLayerProperties(&layer_count, nullptr));
        eastl::vector<vk::LayerProperties> layers {layer_count};
        vkcheck(vk::enumerateInstanceLayerProperties(&layer_count, layers.data()));

        for (const vk::LayerProperties& layer : layers) {
            log_info("Layer present: '{}'", layer.layerName.data());
        }

        // Setup VK_LAYER_KHRONOS_validation layer
        if (m_enable_validation) {
            const char* const k_validation_layer = "VK_LAYER_KHRONOS_validation";
            // Check if this layer is available at instance level
            bool validationLayerPresent = false;
            for (vk::LayerProperties& layer : layers) {
                if (std::strcmp(layer.layerName, k_validation_layer) == 0) {
                    validationLayerPresent = true;
                    break;
                }
            }
            if (validationLayerPresent) [[likely]] {
                instance_create_info.ppEnabledLayerNames = &k_validation_layer;
                instance_create_info.enabledLayerCount = 1;
            } else {
                log_warn("Validation layer {} not present, validation is disabled", k_validation_layer);
            }
        }

        vkcheck(vk::createInstance(&instance_create_info, get_alloc(), &m_instance));

        if (m_enable_validation) {
            m_create_debug_utils_messenger_ext = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(m_instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
            m_destroy_debug_utils_messenger_ext = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(m_instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
            auto* dst = static_cast<VkDebugUtilsMessengerEXT>(m_debug_utils_messenger);
            vkccheck((*m_create_debug_utils_messenger_ext)(
                m_instance,
                &static_cast<VkDebugUtilsMessengerCreateInfoEXT&>(m_debug_utils_messenger_ci),
                reinterpret_cast<const VkAllocationCallbacks*>(vkb::get_alloc()),
                &dst
            ));
            m_debug_utils_messenger = dst;
        }
    }

    auto device::find_physical_device() -> void {
        std::uint32_t num_gpus = 0;
        vkcheck(m_instance.enumeratePhysicalDevices(&num_gpus, nullptr));
        eastl::vector<vk::PhysicalDevice> physical_devices {num_gpus};
        vkcheck(m_instance.enumeratePhysicalDevices(&num_gpus, physical_devices.data()));
        log_info("Found {} Vulkan physical device(s)", num_gpus);
        panic_assert(num_gpus > 0 && "No Vulkan physical devices found");
        std::uint32_t best_device = 0;
        vk::PhysicalDeviceProperties device_properties;
        // Find best GPU, prefer discrete GPU
        for (std::uint32_t i = 0; i < num_gpus; i++) {
            physical_devices[i].getProperties(&device_properties);
            log_info("Found Vulkan physical device: {}", device_properties.deviceName.data());
            if (device_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
                best_device = i;
                break;
            }
        }
        m_physical_device = physical_devices[best_device];
        log_info("Using Vulkan physical device: {}, Type: {}", device_properties.deviceName.data(), string_VkPhysicalDeviceType(static_cast<VkPhysicalDeviceType>(device_properties.deviceType)));
        m_physical_device.getProperties(&m_device_properties);
        m_physical_device.getFeatures(&m_device_features);
        m_physical_device.getMemoryProperties(&m_memory_properties);
        std::uint32_t num_ext = 0;
        vkcheck(m_physical_device.enumerateDeviceExtensionProperties(nullptr, &num_ext, nullptr));
        if (num_ext > 0) [[likely]] {
            m_supported_device_extensions.resize(num_ext);
            if (m_physical_device.enumerateDeviceExtensionProperties(nullptr, &num_ext, m_supported_device_extensions.data()) == vk::Result::eSuccess) {
                for (const vk::ExtensionProperties& extension : m_supported_device_extensions) {
                    log_info("Supported device extension: {}", extension.extensionName.data());
                }
            }
        }

        // print physical device limits
        log_info("---- Physical Device Limits ----");
        dump_physical_device_props(m_device_properties);

        //print memory properties
        log_info("---- Physical GPU Memory Properties ----");
        log_info("Memory Type Count: {}", m_memory_properties.memoryTypeCount);
        for (std::uint32_t i = 0; i < m_memory_properties.memoryTypeCount; i++) {
            log_info("Memory Types[{}] Heap Index: {}", i, m_memory_properties.memoryTypes[i].heapIndex);
            log_info("Memory Types[{}] Property Flags: {:#x}", i, static_cast<std::uint32_t>(m_memory_properties.memoryTypes[i].propertyFlags));
        }
        log_info("Memory Heap Count: {}", m_memory_properties.memoryHeapCount);
        for (std::uint32_t i = 0; i < m_memory_properties.memoryHeapCount; i++) {
            log_info("Memory Heaps[{}] Size: {}", i, m_memory_properties.memoryHeaps[i].size);
            log_info("Memory Heaps[{}] Flags: {:#x}", i, static_cast<std::uint32_t>(m_memory_properties.memoryHeaps[i].flags));
        }

        // Find queue family indices
        std::uint32_t queue_family_count = 0;
        m_physical_device.getQueueFamilyProperties(&queue_family_count, nullptr);
        m_queue_family_properties.resize(queue_family_count);
        m_physical_device.getQueueFamilyProperties(&queue_family_count, m_queue_family_properties.data());
    }

    auto device::create_logical_device(
        const vk::PhysicalDeviceFeatures& enabled_features,
        const eastl::span<const char* const> enabled_extensions,
        void* next_chain,
        vk::QueueFlags requested_queue_types
    ) -> void {
        // Desired queues need to be requested upon logical device creation
        // Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
        // requests different queue types

        eastl::vector<vk::DeviceQueueCreateInfo> queue_infos{};

        // Get queue family indices for the requested queue family types
        // Note that the indices may overlap depending on the implementation

        constexpr float default_queue_priority = 0.0f;

        // Graphics queue
        if (requested_queue_types & vk::QueueFlagBits::eGraphics) {
            m_queue_families.graphics = get_queue_family_index(vk::QueueFlagBits::eGraphics);
            vk::DeviceQueueCreateInfo info {};
            info.queueFamilyIndex = m_queue_families.graphics;
            info.queueCount = 1;
            info.pQueuePriorities = &default_queue_priority;
            queue_infos.emplace_back(info);
        } else {
             m_queue_families.graphics = 0;
        }

        // Dedicated compute queue
        if (requested_queue_types & vk::QueueFlagBits::eCompute) {
            m_queue_families.compute = get_queue_family_index(vk::QueueFlagBits::eCompute);
            if (m_queue_families.compute != m_queue_families.graphics) {
                // If compute family index differs, we need an additional queue create info for the compute queue
                vk::DeviceQueueCreateInfo info {};
                info.queueFamilyIndex = m_queue_families.compute;
                info.queueCount = 1;
                info.pQueuePriorities = &default_queue_priority;
                queue_infos.emplace_back(info);
            }
        } else {
            m_queue_families.compute = m_queue_families.graphics;
        }

        // Dedicated transfer queue
        if (requested_queue_types & vk::QueueFlagBits::eTransfer) {
            m_queue_families.transfer = get_queue_family_index(vk::QueueFlagBits::eTransfer);
            if (m_queue_families.transfer != m_queue_families.graphics
                && m_queue_families.transfer != m_queue_families.compute) {
                // If transfer family index differs, we need an additional queue create info for the transfer queuev
                vk::DeviceQueueCreateInfo info {};
                info.queueFamilyIndex = m_queue_families.transfer;
                info.queueCount = 1;
                info.pQueuePriorities = &default_queue_priority;
                queue_infos.emplace_back(info);
            }
        } else { // Else we use the same queue
            m_queue_families.transfer = m_queue_families.graphics;
        }

        vk::DeviceCreateInfo device_create_info {};
        device_create_info.queueCreateInfoCount = static_cast<std::uint32_t>(queue_infos.size());;
        device_create_info.pQueueCreateInfos = queue_infos.data();
        device_create_info.pEnabledFeatures = &enabled_features;

        // If a pNext(Chain) has been passed, we need to add it to the device creation info
        VkPhysicalDeviceFeatures2 physical_device_features2 {};
        if (next_chain) {
            physical_device_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physical_device_features2.features = enabled_features;
            physical_device_features2.pNext = next_chain;
            device_create_info.pEnabledFeatures = nullptr;
            device_create_info.pNext = &physical_device_features2;
        }

        if (!enabled_extensions.empty()) [[likely]] {
            const auto extension_supported = [this](const char* ext) -> bool {
                return std::ranges::any_of(m_supported_device_extensions, [ext](const vk::ExtensionProperties& extension) {
                    return std::strcmp(extension.extensionName, ext) == 0;
                });
            };
            for (const char* ext : enabled_extensions) {
                if (!extension_supported(ext)) {
                    log_warn("Device extension {} not supported", ext);
                }
            }
            device_create_info.enabledExtensionCount = static_cast<std::uint32_t>(enabled_extensions.size());
            device_create_info.ppEnabledExtensionNames = enabled_extensions.data();
        }

        m_enabled_features = enabled_features;

        log_info("Creating Vulkan logical device...");
        vkcheck(m_physical_device.createDevice(&device_create_info, get_alloc(), &m_logical_device));

        // Fetch queues
        m_logical_device.getQueue(m_queue_families.graphics, 0, &m_graphics_queue);
        m_logical_device.getQueue(m_queue_families.compute, 0, &m_compute_queue);
        m_logical_device.getQueue(m_queue_families.transfer, 0, &m_transfer_queue);
    }

    auto device::get_queue_family_index(const vk::QueueFlags flags) const noexcept -> std::uint32_t {
        const auto& families = m_queue_family_properties;

        // Dedicated queue for compute
        // Try to find a queue family index that supports compute but not graphics
        if ((flags & vk::QueueFlagBits::eCompute) == flags) {
            for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(families.size()); ++i) {
                if ((families[i].queueFlags & vk::QueueFlagBits::eCompute)
                    && static_cast<std::uint32_t>(families[i].queueFlags & vk::QueueFlagBits::eGraphics) == 0) {
                    return i;
                }
            }
        }

        // Dedicated queue for transfer
        // Try to find a queue family index that supports transfer but not graphics and compute
        if ((flags & vk::QueueFlagBits::eTransfer) == flags) {
            for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(families.size()); ++i) {
                if (families[i].queueFlags & vk::QueueFlagBits::eTransfer
                    && static_cast<std::uint32_t>(families[i].queueFlags & vk::QueueFlagBits::eGraphics) == 0
                    && static_cast<std::uint32_t>(families[i].queueFlags & vk::QueueFlagBits::eCompute) == 0) {
                    return i;
                }
            }
        }

        // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
        for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(families.size()); ++i)
            if ((families[i].queueFlags & flags) == flags)
                return i;

        panic("Could not find a matching queue family index");
    }

    auto device::init_vma() -> void {
        log_info("Initializing Vulkan Memory Allocator...");
        VmaAllocatorCreateInfo allocator_info {};
        allocator_info.physicalDevice = m_physical_device;
        allocator_info.device = m_logical_device;
        allocator_info.instance = m_instance;
        allocator_info.pAllocationCallbacks = reinterpret_cast<const VkAllocationCallbacks*>(vkb::get_alloc());
        vkccheck(vmaCreateAllocator(&allocator_info, &m_allocator));
    }

    auto device::find_supported_depth_format(const bool stencil_required, vk::Format& out_format) const -> bool {
        eastl::fixed_vector<vk::Format, 16> formats {};
        if (stencil_required) {
            formats = {
                vk::Format::eD24UnormS8Uint,
                vk::Format::eD32SfloatS8Uint,
                vk::Format::eD16UnormS8Uint
            };
        } else {
            formats = {
                vk::Format::eD24UnormS8Uint,
                vk::Format::eD32SfloatS8Uint,
                vk::Format::eD32Sfloat,
                vk::Format::eD16UnormS8Uint,
                vk::Format::eD16Unorm
            };
        }
        for (const vk::Format format : formats) {
            vk::FormatProperties props;
            m_physical_device.getFormatProperties(format, &props);
            if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
                out_format = format;
                log_info("Found supported depth format: {}", string_VkFormat(static_cast<VkFormat>(format)));
                return true;
            }
        }

        return false;
    }
}
