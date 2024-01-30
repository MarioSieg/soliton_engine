// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "texture.hpp"

#include "../assetmgr/assetmgr.hpp"
#include "../vulkancore/context.hpp"
#include "../vulkancore/buffer.hpp"

#include <bimg/bimg.h>
#include <bimg/decode.h>

namespace graphics {
    static constexpr std::array<vk::Format, static_cast<std::size_t>(bimg::TextureFormat::Count)> map_bimg_format_to_vk {
    };

    texture::texture(const std::string& path) {
        std::vector<std::uint8_t> texels {};
        assetmgr::load_asset_blob_or_panic(asset_category::texture, path, texels);
        passert(texels.size() <= std::numeric_limits<std::uint32_t>::max());
        bimg::ImageContainer* image = bimg::imageParse(nullptr, texels.data(), static_cast<std::uint32_t>(texels.size()));

        //vk::Format format = map_bimg_format(image->m_format);


        bimg::imageFree(image);
    }

    texture::texture(
        const vk::ImageType type,
        const std::uint32_t width,
        const std::uint32_t height,
        const std::uint32_t depth,
        const std::uint32_t mip_levels,
        const std::uint32_t array_size,
        const vk::Format format,
        const VmaMemoryUsage memory_usage,
        const vk::ImageUsageFlags usage,
        const vk::SampleCountFlagBits sample_count,
        const vk::ImageLayout initial_layout,
        const void* data,
        const std::size_t size,
        const vk::ImageCreateFlags flags,
        const vk::ImageTiling tiling
    ) {
        m_width = width;
        m_height = height;
        m_depth = depth;
        m_mip_levels = mip_levels ? mip_levels : static_cast<std::uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
        m_array_size = array_size;
        m_format = format;
        m_usage = usage;
        m_memory_usage = memory_usage;
        m_sample_count = sample_count;
        m_type = type;
        m_flags = flags;
        m_tiling = tiling;
        m_allocator = vkb_device().get_allocator();

        vk::ImageCreateInfo image_info {};
        image_info.imageType = m_type;
        image_info.extent.width = m_width;
        image_info.extent.height = m_height;
        image_info.extent.depth = m_depth;
        image_info.mipLevels = m_mip_levels;
        image_info.arrayLayers = m_array_size;
        image_info.format = m_format;
        image_info.tiling = m_tiling;
        image_info.initialLayout = initial_layout;
        image_info.usage = m_usage;
        image_info.samples = m_sample_count;
        image_info.sharingMode = vk::SharingMode::eExclusive;
        image_info.flags = m_flags;

        VmaAllocationInfo alloc_info {};
        VmaAllocationCreateInfo alloc_create_info {};

        alloc_create_info.usage = m_memory_usage;
        alloc_create_info.flags = memory_usage == VMA_MEMORY_USAGE_CPU_ONLY || memory_usage == VMA_MEMORY_USAGE_GPU_TO_CPU ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0;
        vkccheck(vmaCreateImage(m_allocator, reinterpret_cast<VkImageCreateInfo*>(&image_info), &alloc_create_info, reinterpret_cast<VkImage*>(&m_image), &m_allocation, &alloc_info));

        m_memory = alloc_info.deviceMemory;
        m_mapped = alloc_info.pMappedData;

        if (data && size) [[likely]] {
            const vk::Device device = vkb_vk_device();

            vk::CommandBuffer copy_cmd {};
            vk::CommandBufferAllocateInfo cmd_alloc_info {};
            cmd_alloc_info.commandPool = vkb_context().get_command_pool();
            cmd_alloc_info.level = vk::CommandBufferLevel::ePrimary;
            cmd_alloc_info.commandBufferCount = 1;
            vkcheck(device.allocateCommandBuffers(&cmd_alloc_info, &copy_cmd)); // TODO: not thread safe

            vk::CommandBufferBeginInfo cmd_begin_info {};
            cmd_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
            vkcheck(copy_cmd.begin(&cmd_begin_info));

            vk::ImageSubresourceRange subresource_range {};
            subresource_range.aspectMask = vk::ImageAspectFlagBits::eColor;
            subresource_range.baseMipLevel = 0;
            subresource_range.baseArrayLayer = 0;
            subresource_range.levelCount = m_mip_levels;
            subresource_range.layerCount = m_array_size;

            // Image barrier for optimal image (target)
            // Optimal image will be used as destination for the copy
            set_image_layout_barrier(
                copy_cmd,
                m_image,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                subresource_range
            );

            copy_cmd.end();

            vk::FenceCreateInfo fence_info {};
            vk::Fence fence {};
            vkcheck(device.createFence(&fence_info, &vkb::s_allocator, &fence));

            vk::SubmitInfo submit_info {};
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &copy_cmd;
            vkcheck(vkb_device().get_graphics_queue().submit(1, &submit_info, fence));// TODO: not thread safe, use transfer queue
            vkcheck(device.waitForFences(1, &fence, vk::True, std::numeric_limits<std::uint64_t>::max()));
            device.destroyFence(fence, &vkb::s_allocator);
            device.freeCommandBuffers(vkb_context().get_command_pool(), 1, &copy_cmd);

            upload(0, 0, data, size, vk::ImageLayout::eTransferDstOptimal);

            if (m_mip_levels > 1) {
                generate_mips();
            }
        }
    }

    auto texture::upload(
        const std::size_t array_idx,
        const std::size_t mip_level,
        const void* data,
        const std::size_t size,
        const vk::ImageLayout src_layout,
        const vk::ImageLayout dst_layout
    ) -> void {
        vkb::buffer staging {};
        staging.create(size, 0, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, data);

        vk::BufferImageCopy region {};
        region.bufferOffset = 0;
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = mip_level;
        region.imageSubresource.baseArrayLayer = array_idx;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = m_width;
        region.imageExtent.height = m_height;
        region.imageExtent.depth = 1;

        vk::ImageSubresourceRange subresource_range {};
        subresource_range.aspectMask = vk::ImageAspectFlagBits::eColor;
        subresource_range.baseMipLevel = mip_level;
        subresource_range.levelCount = 1;
        subresource_range.layerCount = 1;
        subresource_range.baseArrayLayer = array_idx;

        const vk::Device device = vkb_vk_device();

        vk::CommandBuffer copy_cmd {};
        vk::CommandBufferAllocateInfo cmd_alloc_info {};
        cmd_alloc_info.commandPool = vkb_context().get_command_pool();
        cmd_alloc_info.level = vk::CommandBufferLevel::ePrimary;
        cmd_alloc_info.commandBufferCount = 1;
        vkcheck(device.allocateCommandBuffers(&cmd_alloc_info, &copy_cmd)); // TODO: not thread safe

        vk::CommandBufferBeginInfo cmd_begin_info {};
        cmd_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        vkcheck(copy_cmd.begin(&cmd_begin_info));

        if (src_layout != vk::ImageLayout::eTransferDstOptimal) {
            set_image_layout_barrier(
                copy_cmd,
                m_image,
                src_layout,
                vk::ImageLayout::eTransferDstOptimal,
                subresource_range
            );
        }
        copy_cmd.copyBufferToImage(
            staging.get_buffer(),
            m_image,
            vk::ImageLayout::eTransferDstOptimal,
            1,
            &region
        );
        if (dst_layout != vk::ImageLayout::eTransferDstOptimal) {
            set_image_layout_barrier(
                copy_cmd,
                m_image,
                vk::ImageLayout::eTransferDstOptimal,
                dst_layout,
                subresource_range
            );
        }

        copy_cmd.end();

        vk::FenceCreateInfo fence_info {};
        vk::Fence fence {};
        vkcheck(device.createFence(&fence_info, &vkb::s_allocator, &fence));

        vk::SubmitInfo submit_info {};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &copy_cmd;
        vkcheck(vkb_device().get_graphics_queue().submit(1, &submit_info, fence));// TODO: not thread safe, use transfer queue
        vkcheck(device.waitForFences(1, &fence, vk::True, std::numeric_limits<std::uint64_t>::max()));
        device.destroyFence(fence, &vkb::s_allocator);
        device.freeCommandBuffers(vkb_context().get_command_pool(), 1, &copy_cmd);
    }

    auto texture::generate_mips(
        const vk::ImageLayout src_layout,
        const vk::ImageLayout dst_layout,
        const vk::ImageAspectFlags aspect_mask,
        const vk::Filter filter
    ) -> void {

    }

    auto texture::set_image_layout_barrier(
        const vk::CommandBuffer cmd_buf,
        const vk::Image image,
        const vk::ImageLayout old_layout,
        const vk::ImageLayout new_layout,
        const vk::ImageSubresourceRange range,
        const vk::PipelineStageFlags src_stage,
        const vk::PipelineStageFlags dst_stage
    ) -> void {
        vk::ImageMemoryBarrier barrier {};
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.image = image;
        barrier.subresourceRange = range;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (old_layout) {
            case vk::ImageLayout::eUndefined: barrier.srcAccessMask = {}; break;
            case vk::ImageLayout::ePreinitialized: barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite; break;
            case vk::ImageLayout::eColorAttachmentOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; break;
            case vk::ImageLayout::eDepthStencilAttachmentOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite; break;
            case vk::ImageLayout::eTransferSrcOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead; break;
            case vk::ImageLayout::eTransferDstOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite; break;
            case vk::ImageLayout::eReadOnlyOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead; break;
            default:
                panic("unsupported layout transition!");
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (new_layout) {
            case vk::ImageLayout::eTransferDstOptimal: barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite; break;
            case vk::ImageLayout::eTransferSrcOptimal: barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead; break;
            case vk::ImageLayout::eColorAttachmentOptimal: barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; break;
            case vk::ImageLayout::eDepthStencilAttachmentOptimal: barrier.dstAccessMask = barrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite; break;
            case vk::ImageLayout::eShaderReadOnlyOptimal:
                if (barrier.srcAccessMask == vk::AccessFlagBits {}) {
                    barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
                }
                barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            break;
            default:
                panic("unsupported layout transition!");
        }

        cmd_buf.pipelineBarrier(
            src_stage,
            dst_stage,
            {},
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );
    }
}
