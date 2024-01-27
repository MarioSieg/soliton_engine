// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "buffer.hpp"
#include "context.hpp"

namespace vkb {
    auto buffer::create(
        const std::size_t size,
        const std::size_t alignment,
        vk::BufferUsageFlags buffer_usage,
        const VmaMemoryUsage memory_usage,
        const VmaAllocationCreateFlags create_flags,
        void* data
    ) -> void {
        m_allocator = vkb_device().get_allocator();
        vk::BufferCreateInfo buffer_create_info {};
        buffer_create_info
            .setSize(size)
            .setSharingMode(vk::SharingMode::eExclusive);
        vk::MemoryPropertyFlags mem_props {};
        switch (memory_usage) {
            case VMA_MEMORY_USAGE_CPU_COPY:
                mem_props |= vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
                buffer_usage |= vk::BufferUsageFlagBits::eTransferSrc;
            break;
            case VMA_MEMORY_USAGE_GPU_ONLY:
                mem_props |= vk::MemoryPropertyFlagBits::eDeviceLocal;
                buffer_usage |= vk::BufferUsageFlagBits::eTransferDst;
            break;
            case VMA_MEMORY_USAGE_CPU_TO_GPU:
                mem_props |= vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
            break;
            case VMA_MEMORY_USAGE_GPU_TO_CPU:
                mem_props |= vk::MemoryPropertyFlagBits::eHostVisible;
            break;
            default:
                panic("Invalid memory usage");
        }
        m_memory_properties = mem_props;
        m_usage = buffer_usage;
        m_memory_usage = memory_usage;
        buffer_create_info.setUsage(buffer_usage);

        VmaAllocationInfo alloc_info {};
        VmaAllocationCreateInfo alloc_create_info {};
        alloc_create_info.usage = memory_usage;
        alloc_create_info.flags = create_flags;
        alloc_create_info.requiredFlags = static_cast<VkMemoryPropertyFlags>(mem_props);
        alloc_create_info.preferredFlags = 0;
        alloc_create_info.memoryTypeBits = 0;
        alloc_create_info.pool = VK_NULL_HANDLE;
        if (!alignment) {
            vkccheck(vmaCreateBuffer(
                m_allocator,
                reinterpret_cast<const VkBufferCreateInfo*>(&buffer_create_info),
                &alloc_create_info,
                reinterpret_cast<VkBuffer*>(&m_buffer),
                &m_allocation,
                &alloc_info
            ));
        } else {
            vkccheck(vmaCreateBufferWithAlignment(
               m_allocator,
               reinterpret_cast<const VkBufferCreateInfo*>(&buffer_create_info),
               &alloc_create_info,
               alignment,
               reinterpret_cast<VkBuffer*>(&m_buffer),
               &m_allocation,
               &alloc_info
            ));
        }
        m_memory = alloc_info.deviceMemory;
        if (create_flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) {
            m_mapped = alloc_info.pMappedData;
        }
        if (data) {
            upload_data(data, size, 0);
        }
        if ((buffer_usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) != vk::BufferUsageFlagBits {}) {
            vk::BufferDeviceAddressInfo buffer_device_address_info {};
            buffer_device_address_info.setBuffer(m_buffer);
            m_device_address = vkb_vk_device().getBufferAddress(buffer_device_address_info);
        }
    }

    auto buffer::destroy() -> void {
        vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
    }

    auto buffer::upload_data(void* data, std::size_t size, std::size_t offset) -> void {
        const vk::Device device = vkb_vk_device();
        if (m_memory_usage == VMA_MEMORY_USAGE_GPU_ONLY) {
            buffer staging_buffer {};
            staging_buffer.create(
                size,
                0,
                vk::BufferUsageFlagBits::eTransferSrc,
                VMA_MEMORY_USAGE_CPU_COPY,
                VMA_ALLOCATION_CREATE_MAPPED_BIT,
                data
            );
            vk::CommandBuffer copy_cmd {};
            vk::CommandBufferAllocateInfo cmd_alloc_info {};
            cmd_alloc_info.commandPool = vkb_context().get_command_pool();
            cmd_alloc_info.level = vk::CommandBufferLevel::ePrimary;
            cmd_alloc_info.commandBufferCount = 1;
            vkcheck(device.allocateCommandBuffers(&cmd_alloc_info, &copy_cmd)); // TODO: not thread safe

            vk::CommandBufferBeginInfo cmd_begin_info {};
            cmd_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
            vkcheck(copy_cmd.begin(&cmd_begin_info));

            vk::BufferCopy copy_region {};
            copy_region.size = size;
            copy_region.dstOffset = offset;
            copy_cmd.copyBuffer(staging_buffer.get_buffer(), m_buffer, 1, &copy_region);
            copy_cmd.end();

            vk::FenceCreateInfo fence_info {};
            vk::Fence fence {};
            vkcheck(device.createFence(&fence_info, &vkb::s_allocator, &fence));

            vk::SubmitInfo submit_info {};
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &copy_cmd;
            vkcheck(vkb_device().get_graphics_queue().submit(1, &submit_info, fence));// TODO: not thread safe
            vkcheck(device.waitForFences(1, &fence, vk::True, std::numeric_limits<std::uint64_t>::max()));
            device.destroyFence(fence, &vkb::s_allocator);
            device.freeCommandBuffers(context::s_instance->get_command_pool(), 1, &copy_cmd);

            staging_buffer.destroy();
        } else {
            if (!m_mapped) {
                vkcheck(device.mapMemory(m_memory, 0, size, {}, &m_mapped));
            }
            std::memcpy(m_mapped, data, size);
            // If host coherency hasn't been requested, do a manual flush to make writes visible
            if (m_memory_properties & vk::MemoryPropertyFlagBits::eHostCoherent) {
                vk::MappedMemoryRange mapped_range {};
                mapped_range.memory = m_memory;
                mapped_range.offset = 0;
                mapped_range.size = vk::WholeSize;
                vkcheck(device.flushMappedMemoryRanges(1, &mapped_range));
            }
        }
    }
}
