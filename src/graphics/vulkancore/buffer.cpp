// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "buffer.hpp"
#include "context.hpp"
#include "command_buffer.hpp"

namespace lu::vkb {
    buffer::buffer(
        const std::size_t size,
        const std::size_t alignment,
        vk::BufferUsageFlags buffer_usage,
        const VmaMemoryUsage memory_usage,
        const VmaAllocationCreateFlags create_flags,
        const void* const data
    ) {
        m_allocator = vkb::dvc().get_allocator();
        m_size = size;
        vk::BufferCreateInfo buffer_create_info {};
        buffer_create_info.size = size;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;
        vk::MemoryPropertyFlags mem_props {};
        switch (memory_usage) {
            case VMA_MEMORY_USAGE_CPU_ONLY:
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
        buffer_create_info.usage = buffer_usage;

        VmaAllocationInfo alloc_info {};
        VmaAllocationCreateInfo alloc_create_info {};
        alloc_create_info.usage = memory_usage;
        alloc_create_info.flags = create_flags;
        alloc_create_info.requiredFlags = static_cast<VkMemoryPropertyFlags>(mem_props);
        alloc_create_info.preferredFlags = 0;
        alloc_create_info.memoryTypeBits = 0;
        alloc_create_info.pool = nullptr;
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
            buffer_device_address_info.buffer = m_buffer;
            m_device_address = vkb::vkdvc().getBufferAddress(&buffer_device_address_info);
        }
    }

    buffer::~buffer() {
        if (m_buffer) {
            vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
            m_buffer = nullptr;
        }
    }

    auto buffer::upload_data(const void* data, const std::size_t size, const std::size_t offset) -> void {
        const vk::Device device = vkb::vkdvc();
        if (m_memory_usage == VMA_MEMORY_USAGE_GPU_ONLY) {
            buffer staging_buffer {
                size,
                0,
                vk::BufferUsageFlagBits::eTransferSrc,
                VMA_MEMORY_USAGE_CPU_ONLY,
                VMA_ALLOCATION_CREATE_MAPPED_BIT,
                data
            };

            command_buffer copy_cmd {vk::QueueFlagBits::eTransfer};
            copy_cmd.begin();
            copy_cmd.copy_buffer(staging_buffer.get_buffer(), m_buffer, size, offset);
            copy_cmd.end();
            copy_cmd.flush();
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
