// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "texture.hpp"

#include "../assetmgr/assetmgr.hpp"
#include "vulkancore/context.hpp"
#include "vulkancore/buffer.hpp"

#include <bx/allocator.h>
#include <bimg/bimg.h>
#include <bimg/decode.h>

namespace graphics {
    [[nodiscard]] static auto map_bimg_format_to_vk(const bimg::TextureFormat::Enum fmt) noexcept -> vk::Format {
        using enum bimg::TextureFormat::Enum;
        switch(fmt) {
            case BC1: return vk::Format::eBc1RgbUnormBlock;
            case BC2: return vk::Format::eBc2UnormBlock;
            case BC3: return vk::Format::eBc3UnormBlock;
            case BC4: return vk::Format::eBc4UnormBlock;
            case BC5: return vk::Format::eBc5UnormBlock;
            case BC6H: return vk::Format::eBc6HUfloatBlock;
            case BC7: return vk::Format::eBc7UnormBlock;
            case ETC1: panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case ETC2: return vk::Format::eEtc2R8G8B8UnormBlock;
            case ETC2A: return vk::Format::eEtc2R8G8B8A8UnormBlock;
            case ETC2A1: return vk::Format::eEtc2R8G8B8A1UnormBlock;
            case PTC12:  panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case PTC14:  panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case PTC12A: panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case PTC14A: panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case PTC22:  panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case PTC24:  panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case ATC:    panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case ATCE:   panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case ATCI:   panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case ASTC4x4: return vk::Format::eAstc4x4UnormBlock;
            case ASTC5x4: return vk::Format::eAstc5x4UnormBlock;
            case ASTC5x5: return vk::Format::eAstc5x5UnormBlock;
            case ASTC6x5: return vk::Format::eAstc6x5UnormBlock;
            case ASTC6x6: return vk::Format::eAstc6x6UnormBlock;
            case ASTC8x5: return vk::Format::eAstc8x5UnormBlock;
            case ASTC8x6: return vk::Format::eAstc8x6UnormBlock;
            case ASTC8x8: return vk::Format::eAstc8x8UnormBlock;
            case ASTC10x5: return vk::Format::eAstc10x5UnormBlock;
            case ASTC10x6: return vk::Format::eAstc10x6UnormBlock;
            case ASTC10x8: return vk::Format::eAstc10x8UnormBlock;
            case ASTC10x10: return vk::Format::eAstc10x10UnormBlock;
            case ASTC12x10: return vk::Format::eAstc12x10UnormBlock;
            case ASTC12x12: return vk::Format::eAstc12x12UnormBlock;
            case R1: panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case A8:  panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case R8: return vk::Format::eR8Unorm;
            case R8I: return vk::Format::eR8Sint;
            case R8U: return vk::Format::eR8Uint;
            case R8S: return vk::Format::eR8Snorm;
            case R16: return vk::Format::eR16Unorm;
            case R16I: return vk::Format::eR16Sint;
            case R16U: return vk::Format::eR16Uint;
            case R16F: return vk::Format::eR16Sfloat;
            case R16S: return vk::Format::eR16Snorm;
            case R32I: return vk::Format::eR32Sint;
            case R32U: return vk::Format::eR32Uint;
            case R32F: return vk::Format::eR32Sfloat;
            case RG8: return vk::Format::eR8G8Unorm;
            case RG8I: return vk::Format::eR8G8Sint;
            case RG8U: return vk::Format::eR8G8Uint;
            case RG8S: return vk::Format::eR8G8Snorm;
            case RG16: return vk::Format::eR16G16Unorm;
            case RG16I: return vk::Format::eR16G16Sint;
            case RG16U: return vk::Format::eR16G16Uint;
            case RG16F: return vk::Format::eR16G16Sfloat;
            case RG16S: return vk::Format::eR16G16Snorm;
            case RG32I: return vk::Format::eR32G32Sint;
            case RG32U: return vk::Format::eR32G32Uint;
            case RG32F: return vk::Format::eR32G32Sfloat;
            case RGB8: return vk::Format::eR8G8B8Unorm;
            case RGB8I: return vk::Format::eR8G8B8Sint;
            case RGB8U: return vk::Format::eR8G8B8Uint;
            case RGB8S: return vk::Format::eR8G8B8Snorm;
            case RGB9E5F: return vk::Format::eE5B9G9R9UfloatPack32;
            case BGRA8: return vk::Format::eB8G8R8A8Unorm;
            case RGBA8: return vk::Format::eR8G8B8A8Unorm;
            case RGBA8I: return vk::Format::eR8G8B8A8Sint;
            case RGBA8U: return vk::Format::eR8G8B8A8Uint;
            case RGBA8S: return vk::Format::eR8G8B8A8Snorm;
            case RGBA16: return vk::Format::eR16G16B16A16Unorm;
            case RGBA16I: return vk::Format::eR16G16B16A16Sint;
            case RGBA16U: return vk::Format::eR16G16B16A16Uint;
            case RGBA16F: return vk::Format::eR16G16B16A16Sfloat;
            case RGBA16S: return vk::Format::eR16G16B16A16Snorm;
            case RGBA32I: return vk::Format::eR32G32B32A32Sint;
            case RGBA32U: return vk::Format::eR32G32B32A32Uint;
            case RGBA32F: return vk::Format::eR32G32B32A32Sfloat;
            case B5G6R5: return vk::Format::eB5G6R5UnormPack16;
            case R5G6B5: return vk::Format::eR5G6B5UnormPack16;
            case BGRA4: return vk::Format::eB4G4R4A4UnormPack16;
            case RGBA4: return vk::Format::eR4G4B4A4UnormPack16;
            case BGR5A1: return vk::Format::eB5G5R5A1UnormPack16;
            case RGB5A1: return vk::Format::eR5G5B5A1UnormPack16;
            case RGB10A2: return vk::Format::eA2R10G10B10UnormPack32;
            case RG11B10F: return vk::Format::eB10G11R11UfloatPack32;
            case D16:  panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case D24:  panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case D24S8: return vk::Format::eD24UnormS8Uint;
            case D32: return vk::Format::eD32Sfloat;
            case D16F:  panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case D24F:  panic("Currently unsupported texture format: {}", bimg::getName(fmt));
            case D32F: return vk::Format::eD32Sfloat;
            case D0S8: return vk::Format::eS8Uint;
            default:  panic("Currently unsupported texture format: {}", bimg::getName(fmt));
        }
    }

    class allocator final : public bx::AllocatorI {
    public:
        auto realloc(void* p, size_t size, size_t align, const char* filePath, std::uint32_t line) -> void* override;
    };

    static constexpr std::size_t k_natural_align = 8;
    auto allocator::realloc(void* p, std::size_t size, std::size_t align, const char*, std::uint32_t) -> void* {
        if (0 == size) {
            if (nullptr != p) {
                if (k_natural_align >= align) {
                    mi_free(p);
                    return nullptr;
                }
                mi_free_aligned(p, align);
            }
            return nullptr;
        }
        if (nullptr == p) {
            if (k_natural_align >= align) {
                return mi_malloc(size);
            }
            return mi_malloc_aligned(size, align);
        }
        if (k_natural_align >= align) {
            return mi_realloc(p, size);
        }
        return mi_realloc_aligned(p, size, align);
    }

    static constinit allocator s_allocator {};

    texture::texture(std::string&& asset_path) : asset { asset_category::texture, asset_source::filesystem, std::move(asset_path) } {
        std::vector<std::uint8_t> texels {};
        assetmgr::load_asset_blob_or_panic(asset_category::texture, get_asset_path(), texels);
        passert(texels.size() <= std::numeric_limits<std::uint32_t>::max());
        bimg::ImageContainer* image = bimg::imageParse(&s_allocator, texels.data(), static_cast<std::uint32_t>(texels.size()));

        const vk::Format format = map_bimg_format_to_vk(image->m_format);
        create(
            vk::ImageType::e2D, // TODO: cubemap?
            image->m_width,
            image->m_height,
            image->m_depth,
            image->m_numMips <= 1 ? 0 : image->m_numMips,
            image->m_numLayers,
            format,
            VMA_MEMORY_USAGE_GPU_ONLY,
            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc,
            vk::SampleCountFlagBits::e1,
            vk::ImageLayout::eUndefined,
            image->m_data,
            image->m_size
        );

        m_approx_byte_size = sizeof(*this) + image->m_size;

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
    ) : asset {asset_category::texture, asset_source::memory} {
        create(
            type,
            width,
            height,
            depth,
            mip_levels,
            array_size,
            format,
            memory_usage,
            usage,
            sample_count,
            initial_layout,
            data,
            size,
            flags,
            tiling
        );
    }

    texture::~texture() {
        if (m_image) {
            vkb_vk_device().destroyImageView(m_image_view, &vkb::s_allocator);
            vmaDestroyImage(m_allocator, m_image, m_allocation);
            m_image = nullptr;
        }
    }

    auto texture::create(
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
    ) -> void {
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
        m_layout = initial_layout;
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
            const vk::CommandBuffer copy_cmd = vkb_context().start_command_buffer<vk::QueueFlagBits::eTransfer>();

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

            vkb_context().flush_command_buffer<vk::QueueFlagBits::eTransfer>(copy_cmd);

            upload(0, 0, data, size, vk::ImageLayout::eTransferDstOptimal);

            if (m_mip_levels > 1) {
                generate_mips();
            }
        }

        // image view:
        vk::ImageViewCreateInfo image_view_ci {};
        image_view_ci.viewType = vk::ImageViewType::e2D;
        image_view_ci.format = m_format;
        image_view_ci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        image_view_ci.subresourceRange.baseMipLevel = 0;
        image_view_ci.subresourceRange.baseArrayLayer = 0;
        image_view_ci.subresourceRange.layerCount = m_array_size;
        image_view_ci.subresourceRange.levelCount = m_mip_levels;
        image_view_ci.image = m_image;
        vkcheck(vkb_vk_device().createImageView(&image_view_ci, &vkb::s_allocator, &m_image_view));
    }

    auto texture::upload(
        const std::size_t array_idx,
        const std::size_t mip_level,
        const void* data,
        const std::size_t size,
        const vk::ImageLayout src_layout,
        const vk::ImageLayout dst_layout
    ) const -> void {
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

        const vk::CommandBuffer copy_cmd = vkb_context().start_command_buffer<vk::QueueFlagBits::eTransfer>();

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

        vkb_context().flush_command_buffer<vk::QueueFlagBits::eTransfer>(copy_cmd);
    }

    auto texture::generate_mips(
        const vk::ImageLayout src_layout,
        const vk::ImageLayout dst_layout,
        const vk::ImageAspectFlags aspect_mask,
        const vk::Filter filter
    ) const -> void {
        const vk::CommandBuffer blit_cmd = vkb_context().start_command_buffer<vk::QueueFlagBits::eGraphics>();

        vk::ImageSubresourceRange intial_subresource_range {};
        intial_subresource_range.aspectMask = aspect_mask;
        intial_subresource_range.baseMipLevel = 1;
        intial_subresource_range.levelCount = m_mip_levels - 1;
        intial_subresource_range.layerCount = m_array_size;
        intial_subresource_range.baseArrayLayer = 0;

        set_image_layout_barrier(
            blit_cmd,
            m_image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            intial_subresource_range
        );

        vk::ImageSubresourceRange subresource_range {};
        subresource_range.aspectMask = aspect_mask;
        subresource_range.levelCount = 1;
        subresource_range.layerCount = 1;

        for (std::uint32_t i = 0; i < m_array_size; ++i) {
            std::uint32_t mip_w = m_width;
            std::uint32_t mip_h = m_height;
            for (std::uint32_t j = 1; j < m_mip_levels; ++j) {
                subresource_range.baseMipLevel = j - 1;
                subresource_range.baseArrayLayer = i;
                vk::ImageLayout layout = vk::ImageLayout::eTransferDstOptimal;
                if (j == 1) {
                    layout = src_layout;
                }
                if (layout != vk::ImageLayout::eTransferSrcOptimal) {
                    set_image_layout_barrier(
                        blit_cmd,
                        m_image,
                        layout,
                        vk::ImageLayout::eTransferSrcOptimal,
                        subresource_range
                    );
                }
                vk::ImageBlit blit {};
                blit.srcOffsets[0] = vk::Offset3D { 0, 0, 0 };
                blit.srcOffsets[1] = vk::Offset3D { static_cast<std::int32_t>(mip_w), static_cast<std::int32_t>(mip_h), 1 };
                blit.srcSubresource.aspectMask = aspect_mask;
                blit.srcSubresource.mipLevel = j - 1;
                blit.srcSubresource.baseArrayLayer = i;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = vk::Offset3D { 0, 0, 0 };
                blit.dstOffsets[1] = vk::Offset3D { static_cast<std::int32_t>(mip_w > 1 ? mip_w >> 1 : 1), static_cast<std::int32_t>(mip_h > 1 ? mip_h >> 1 : 1), 1 };
                blit.dstSubresource.aspectMask = aspect_mask;
                blit.dstSubresource.mipLevel = j;
                blit.dstSubresource.baseArrayLayer = i;
                blit.dstSubresource.layerCount = 1;
                blit_cmd.blitImage(
                    m_image,
                    vk::ImageLayout::eTransferSrcOptimal,
                    m_image,
                    vk::ImageLayout::eTransferDstOptimal,
                    1,
                    &blit,
                    filter
                );
                set_image_layout_barrier(
                    blit_cmd,
                    m_image,
                    vk::ImageLayout::eTransferSrcOptimal,
                    dst_layout,
                    subresource_range
                );
                if (mip_w > 1) {
                    mip_w >>= 1;
                }
                if (mip_h > 1) {
                    mip_h >>= 1;
                }
            }
        }

        subresource_range.baseMipLevel = m_mip_levels - 1;
        set_image_layout_barrier(
            blit_cmd,
            m_image,
            vk::ImageLayout::eTransferDstOptimal,
            dst_layout,
            subresource_range
        );

        vkb_context().flush_command_buffer<vk::QueueFlagBits::eGraphics>(blit_cmd);
        log_info("Generated mipchain with {} maps", m_mip_levels);
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
            case vk::ImageLayout::eShaderReadOnlyOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead; break;
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
