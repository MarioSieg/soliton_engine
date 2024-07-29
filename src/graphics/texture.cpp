// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "texture.hpp"

#include "../assetmgr/assetmgr.hpp"
#include "vulkancore/context.hpp"
#include "vulkancore/buffer.hpp"

#include <bimg/bimg.h>
#include <bimg/decode.h>
#include <Simd/SimdLib.hpp>

namespace lu::graphics {
    // Textures are converted to this format when native format is not supported on the GPU
    static constexpr bimg::TextureFormat::Enum k_fallback_format = bimg::TextureFormat::RGBA8;

    /*
     * Enables SIMD conversion for common formats (RGB8 -> RGBA8) using runtime CPU detection and vectorized algorithms.
     * This makes the importing of external scenes with raw .png or .jpg images noteably faster.
     * Example (a big interior scene with some raw images) (RELEASE):
     * ON: 1.98S
     * OFF: 2.54s
     * In DEBUG mode the difference is even bigger, which allows for faster iteration times:
     * ON: 6.19s
     * OFF: 18.2s
     */
    static constexpr bool k_enable_simd_cvt = true;

    struct texture_format_info final {
        VkFormat fmt {};
        VkFormat fmt_srv {};
        VkFormat fmt_dsv {};
        VkFormat fmt_srgb {};
        VkComponentMapping mapping {};
    };
    extern const std::array<texture_format_info, 96> k_texture_format_map;

#if USE_MIMALLOC
    static constexpr std::size_t k_natural_align = 8;
    auto texture_allocator::realloc(void* p, const std::size_t size, const std::size_t align, const char*, std::uint32_t) -> void* {
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

    constinit texture_allocator s_texture_allocator {};
#else
    static bx::DefaultAllocator s_def_texture_allocator {};
#endif

    [[nodiscard]] consteval auto get_tex_alloc() noexcept -> bx::AllocatorI* {
#if USE_MIMALLOC
        return &s_texture_allocator;
#else
        return &s_def_texture_allocator;
#endif
    }

    texture::texture(std::string&& asset_path) : asset {assetmgr::asset_source::filesystem, std::move(asset_path)} {
        log_info("Loading texture '{}'", get_asset_path());
        std::vector<std::byte> texels {};
        assetmgr::with_primary_accessor_lock([&](assetmgr::asset_accessor &acc) {
            if (!acc.load_bin_file(get_asset_path().c_str(), texels)) {
                panic("Failed to load texture '{}'", get_asset_path());
            }
        });
        parse_from_raw_memory(texels);
    }

    texture::texture(const std::span<const std::byte> raw_mem) : asset {assetmgr::asset_source::memory} {
        parse_from_raw_memory(raw_mem);
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
    ) : asset {assetmgr::asset_source::memory} {
        create(
            nullptr,
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
            vkb::vkdvc().destroySampler(m_sampler, vkb::get_alloc());
            vkb::vkdvc().destroyImageView(m_image_view, vkb::get_alloc());
            vmaDestroyImage(m_allocator, m_image, m_allocation);
            m_image = nullptr;
        }
    }

    auto texture::create(
        void* img,
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
        m_allocator = vkb::dvc().get_allocator();

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
        vkccheck(vmaCreateImage(
            m_allocator,
            reinterpret_cast<VkImageCreateInfo*>(&image_info),
            &alloc_create_info,
            reinterpret_cast<VkImage*>(&m_image),
            &m_allocation,
            &alloc_info
        ));

        m_memory = alloc_info.deviceMemory;
        m_mapped = alloc_info.pMappedData;

        if (data && size) [[likely]] {
            const vk::CommandBuffer copy_cmd = vkb::ctx().start_command_buffer<vk::QueueFlagBits::eTransfer>();

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

            vkb::ctx().flush_command_buffer<vk::QueueFlagBits::eTransfer>(copy_cmd);

            upload(img, 0, 0, data, size, vk::ImageLayout::eTransferDstOptimal);

            if (m_mip_levels > 1) {
                // if mip_levels == 1, we can just use the image as-is and not generate mipmaps but only barrier them
                const bool transfer_only = mip_levels > 1;
                generate_mips(transfer_only);
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
        vkcheck(vkb::vkdvc().createImageView(&image_view_ci, vkb::get_alloc(), &m_image_view));

        create_sampler();
    }

    auto texture::parse_from_raw_memory(const std::span<const std::byte> texels) -> void {
        passert(texels.size() <= std::numeric_limits<std::uint32_t>::max());
        bimg::ImageContainer* image = bimg::imageParse(get_tex_alloc(), texels.data(), static_cast<std::uint32_t>(texels.size()));
        passert(image != nullptr);

        constexpr vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
        constexpr vk::ImageCreateFlags create_flags {};
        constexpr vk::ImageTiling tiling = vk::ImageTiling::eOptimal;

        auto format = static_cast<vk::Format>(k_texture_format_map[image->m_format].fmt);
        log_info("Texture format: {}", string_VkFormat(static_cast<VkFormat>(format)));
        bool is_format_supported = false;
        bool is_format_mipgen_supported = false;
        vkb::dvc().is_image_format_supported(
            vk::ImageType::e2D,
            format,
            create_flags,
            usage,
            tiling,
            is_format_supported,
            is_format_mipgen_supported
        );
        bool conversion_required = !is_format_supported; // if the hardware doesn't support the format, we'll convert it to a supported format
        if (!is_format_supported || (image->m_numMips <= 1 && !is_format_mipgen_supported)) {
            const auto now = std::chrono::high_resolution_clock::now();

            bimg::ImageContainer* original = image;
            if (k_enable_simd_cvt && original->m_format == bimg::TextureFormat::RGB8) { // User faster SIMD for conversion of common formats
                static_assert(k_fallback_format == bimg::TextureFormat::RGBA8, "Fallback format must be RGBA8 for SIMD conversion");
                image = bimg::imageAlloc(
                    get_tex_alloc(),
                    k_fallback_format,
                    original->m_width,
                    original->m_height,
                    original->m_depth,
                    original->m_numLayers,
                    original->m_cubeMap,
                    original->m_numMips > 1
                );
                const std::uint16_t sides = original->m_numLayers * (original->m_cubeMap ? 6 : 1);
                for (std::uint16_t side = 0; side < sides; ++side) {
                    for (std::uint8_t lod = 0, num = original->m_numMips; lod < num; ++lod) {
                        bimg::ImageMip src_mip {};
                        if (bimg::imageGetRawData(*original, side, lod, original->m_data, original->m_size, src_mip)) [[likely]] {
                            bimg::ImageMip dst_mip {};
                            bimg::imageGetRawData(*image, side, lod, image->m_data, image->m_size, dst_mip);
                            const std::uint8_t* const src = src_mip.m_data;
                            std::uint8_t* const dst = const_cast<std::uint8_t*>(dst_mip.m_data);
                            SimdRgbToBgra(
                                src,
                                src_mip.m_width,
                                src_mip.m_height,
                                src_mip.m_width * 3,
                                dst,
                                dst_mip.m_width * 4,
                                0xff
                            );
                            SimdBgraToRgba(
                                dst,
                                dst_mip.m_width,
                                dst_mip.m_height,
                                dst_mip.m_width * 4,
                                dst,
                                dst_mip.m_width * 4
                            );
                        } else {
                            log_warn("Failed to get raw data for mip level {}", lod);
                        }
                    }
                }
            } else {
                image = bimg::imageConvert(get_tex_alloc(), k_fallback_format, *original, true);
                passert(image != nullptr);
            }
            bimg::imageFree(original);
            original = nullptr;
            log_warn(
                "Texture format not supported by GPU: {} converted to fallback format {} in {:.03f}ms",
                string_VkFormat(static_cast<VkFormat>(format)),
                string_VkFormat(k_texture_format_map[k_fallback_format].fmt),
                std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - now).count()
            );
            format = static_cast<vk::Format>(k_texture_format_map[k_fallback_format].fmt);
        }
        const exit_guard img_free {
            [&image] () -> void {
                bimg::imageFree(image);
                image = nullptr;
            }
        };

        create(
            image,
            vk::ImageType::e2D, // TODO: Support for cubemap / volume texture (texture 3d)
            image->m_width,
            image->m_height,
            image->m_depth,
            image->m_numMips <= 1 ? 0 : image->m_numMips,
            image->m_numLayers,
            format,
            VMA_MEMORY_USAGE_GPU_ONLY,
            usage,
            vk::SampleCountFlagBits::e1,
            vk::ImageLayout::eUndefined,
            image->m_data,
            image->m_size,
            create_flags,
            tiling
        );

        m_approx_byte_size = sizeof(*this) + image->m_size;
    }

    auto texture::upload(
        void* img,
        const std::size_t array_idx,
        const std::size_t mip_level,
        const void* data,
        const std::size_t size,
        const vk::ImageLayout src_layout,
        const vk::ImageLayout dst_layout
    ) const -> void {
        vkb::buffer staging {};
        staging.create(size, 0, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, data);

        vk::ImageSubresourceRange subresource_range {};
        subresource_range.aspectMask = vk::ImageAspectFlagBits::eColor;
        subresource_range.baseMipLevel = mip_level;
        subresource_range.levelCount = m_mip_levels;
        subresource_range.layerCount = 1;
        subresource_range.baseArrayLayer = array_idx;

        const vk::CommandBuffer copy_cmd = vkb::ctx().start_command_buffer<vk::QueueFlagBits::eTransfer>();

        if (src_layout != vk::ImageLayout::eTransferDstOptimal) {
            set_image_layout_barrier(
                copy_cmd,
                m_image,
                src_layout,
                vk::ImageLayout::eTransferDstOptimal,
                subresource_range
            );
        }
        // copy all mip levels
        std::vector<vk::BufferImageCopy> regions {};
        regions.reserve(m_mip_levels);
        std::size_t offset = 0;
        // To remove loop and replace with sub-viewport blitting inside VRAM (safes m_mip_levels-1 copies from host to GPU)
        for (std::uint32_t i = 0; i < m_mip_levels; ++i) {
            passert(img);
            auto* image = static_cast<bimg::ImageContainer*>(img);
            bimg::ImageMip mip {};
            if (!bimg::imageGetRawData(*image, 0, i, data, size, mip)) [[unlikely]] {
                log_warn("Failed to get raw data for mip level {}", i);
                // TODO: What to do? Fill mip-level pyramid memory with a solid color?
                continue;
            }
            vk::BufferImageCopy region {};
            region.bufferOffset = offset;
            offset += mip.m_size;
            region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            region.imageSubresource.mipLevel = i;
            region.imageSubresource.baseArrayLayer = array_idx;
            region.imageSubresource.layerCount = 1;
            region.imageExtent.width = m_width >> i;
            region.imageExtent.height = m_height >> i;
            region.imageExtent.depth = 1;
            regions.emplace_back(region);
        }
        copy_cmd.copyBufferToImage( // TODO: replace with single big blit (if possible)
            staging.get_buffer(),
            m_image,
            vk::ImageLayout::eTransferDstOptimal,
            regions.size(),
            regions.data()
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

        vkb::ctx().flush_command_buffer<vk::QueueFlagBits::eTransfer>(copy_cmd);
    }

    auto texture::generate_mips(
        const bool transfer_only,
        const vk::ImageLayout src_layout,
        const vk::ImageLayout dst_layout,
        const vk::ImageAspectFlags aspect_mask,
        const vk::Filter filter
    ) const -> void {
        const vk::CommandBuffer blit_cmd = vkb::ctx().start_command_buffer<vk::QueueFlagBits::eGraphics>();

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
                if (!transfer_only) { // If we're only doing a transfer, we don't need to blit. TODO: refractor
                    vk::ImageBlit blit {};
                    blit.srcOffsets[0] = vk::Offset3D { 0, 0, 0 };
                    blit.srcOffsets[1] = vk::Offset3D { static_cast<std::int32_t>(mip_w), static_cast<std::int32_t>(mip_h), 1 };
                    blit.srcSubresource.aspectMask = aspect_mask;
                    blit.srcSubresource.mipLevel = j - 1;
                    blit.srcSubresource.baseArrayLayer = i;
                    blit.srcSubresource.layerCount = 1;
                    blit.dstOffsets[0] = vk::Offset3D { 0, 0, 0 };
                    blit.dstOffsets[1] = vk::Offset3D {
                        static_cast<std::int32_t>(mip_w > 1 ? mip_w >> 1 : 1),
                        static_cast<std::int32_t>(mip_h > 1 ? mip_h >> 1 : 1),
                        1
                    };
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
                }
                set_image_layout_barrier(
                    blit_cmd,
                    m_image,
                    vk::ImageLayout::eTransferSrcOptimal,
                    dst_layout,
                    subresource_range
                );
                mip_w >>= (mip_w > 1);
                mip_h >>= (mip_h > 1);
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

        vkb::ctx().flush_command_buffer<vk::QueueFlagBits::eGraphics>(blit_cmd);
        log_info("{} mip-chain with {} maps", transfer_only ? "Loaded" : "Generated", m_mip_levels);
    }

    auto texture::create_sampler() -> void {
        const bool supports_anisotropy = vkb::dvc().get_physical_device_features().samplerAnisotropy;

        vk::SamplerCreateInfo sampler_info {};
        sampler_info.magFilter = vk::Filter::eLinear;
        sampler_info.minFilter = vk::Filter::eLinear;
        sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
        sampler_info.addressModeU = vk::SamplerAddressMode::eRepeat;
        sampler_info.addressModeV = vk::SamplerAddressMode::eRepeat;
        sampler_info.addressModeW = vk::SamplerAddressMode::eRepeat;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.compareOp = vk::CompareOp::eAlways;
        sampler_info.compareEnable = vk::False;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = static_cast<float>(m_mip_levels);
        sampler_info.maxAnisotropy = supports_anisotropy ? vkb::dvc().get_physical_device_props().limits.maxSamplerAnisotropy : 1.0f;
        sampler_info.anisotropyEnable = supports_anisotropy ? vk::True : vk::False;
        sampler_info.borderColor = vk::BorderColor::eFloatOpaqueBlack;
        sampler_info.unnormalizedCoordinates = vk::False;
        vkcheck(vkb::vkdvc().createSampler(&sampler_info, vkb::get_alloc(), &m_sampler));
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

    const std::array<texture_format_info, 96> k_texture_format_map = {
        #define $_ VK_COMPONENT_SWIZZLE_IDENTITY
        #define $0 VK_COMPONENT_SWIZZLE_ZERO
        #define $1 VK_COMPONENT_SWIZZLE_ONE
        #define $R VK_COMPONENT_SWIZZLE_R
        #define $G VK_COMPONENT_SWIZZLE_G
        #define $B VK_COMPONENT_SWIZZLE_B
        #define $A VK_COMPONENT_SWIZZLE_A
        texture_format_info { VK_FORMAT_BC1_RGB_UNORM_BLOCK,       VK_FORMAT_BC1_RGB_UNORM_BLOCK,      VK_FORMAT_UNDEFINED,           VK_FORMAT_BC1_RGB_SRGB_BLOCK,       { $_, $_, $_, $_ } }, // BC1
        texture_format_info { VK_FORMAT_BC2_UNORM_BLOCK,           VK_FORMAT_BC2_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_BC2_SRGB_BLOCK,           { $_, $_, $_, $_ } }, // BC2
        texture_format_info { VK_FORMAT_BC3_UNORM_BLOCK,           VK_FORMAT_BC3_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_BC3_SRGB_BLOCK,           { $_, $_, $_, $_ } }, // BC3
        texture_format_info { VK_FORMAT_BC4_UNORM_BLOCK,           VK_FORMAT_BC4_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // BC4
        texture_format_info { VK_FORMAT_BC5_UNORM_BLOCK,           VK_FORMAT_BC5_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // BC5
        texture_format_info { VK_FORMAT_BC6H_SFLOAT_BLOCK,         VK_FORMAT_BC6H_SFLOAT_BLOCK,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // BC6H
        texture_format_info { VK_FORMAT_BC7_UNORM_BLOCK,           VK_FORMAT_BC7_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_BC7_SRGB_BLOCK,           { $_, $_, $_, $_ } }, // BC7
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // ETC1
        texture_format_info { VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,   VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,   { $_, $_, $_, $_ } }, // ETC2
        texture_format_info { VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, { $_, $_, $_, $_ } }, // ETC2A
        texture_format_info { VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, { $_, $_, $_, $_ } }, // ETC2A1
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC12
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC14
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC12A
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC14A
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC22
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC24
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // ATC
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // ATCE
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // ATCI
        texture_format_info { VK_FORMAT_ASTC_4x4_UNORM_BLOCK,      VK_FORMAT_ASTC_4x4_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_4x4_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC4x4
        texture_format_info { VK_FORMAT_ASTC_5x4_UNORM_BLOCK,      VK_FORMAT_ASTC_5x4_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_5x4_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC5x4
        texture_format_info { VK_FORMAT_ASTC_5x5_UNORM_BLOCK,      VK_FORMAT_ASTC_5x5_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_5x5_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC5x5
        texture_format_info { VK_FORMAT_ASTC_6x5_UNORM_BLOCK,      VK_FORMAT_ASTC_6x5_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_6x5_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC6x5
        texture_format_info { VK_FORMAT_ASTC_6x6_UNORM_BLOCK,      VK_FORMAT_ASTC_6x6_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_6x6_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC6x6
        texture_format_info { VK_FORMAT_ASTC_8x5_UNORM_BLOCK,      VK_FORMAT_ASTC_8x5_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_8x5_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC8x5
        texture_format_info { VK_FORMAT_ASTC_8x6_UNORM_BLOCK,      VK_FORMAT_ASTC_8x6_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_8x6_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC8x6
        texture_format_info { VK_FORMAT_ASTC_8x8_UNORM_BLOCK,      VK_FORMAT_ASTC_8x8_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_8x8_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC8x8
        texture_format_info { VK_FORMAT_ASTC_10x5_UNORM_BLOCK,     VK_FORMAT_ASTC_10x5_UNORM_BLOCK,    VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_10x5_SRGB_BLOCK,     { $_, $_, $_, $_ } }, // ASTC10x5
        texture_format_info { VK_FORMAT_ASTC_10x6_UNORM_BLOCK,     VK_FORMAT_ASTC_10x6_UNORM_BLOCK,    VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_10x6_SRGB_BLOCK,     { $_, $_, $_, $_ } }, // ASTC10x6
        texture_format_info { VK_FORMAT_ASTC_10x8_UNORM_BLOCK,     VK_FORMAT_ASTC_10x8_UNORM_BLOCK,    VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_10x8_SRGB_BLOCK,     { $_, $_, $_, $_ } }, // ASTC10x8
        texture_format_info { VK_FORMAT_ASTC_10x10_UNORM_BLOCK,    VK_FORMAT_ASTC_10x10_UNORM_BLOCK,   VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_10x10_SRGB_BLOCK,    { $_, $_, $_, $_ } }, // ASTC10x10
        texture_format_info { VK_FORMAT_ASTC_12x10_UNORM_BLOCK,    VK_FORMAT_ASTC_12x10_UNORM_BLOCK,   VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_12x10_SRGB_BLOCK,    { $_, $_, $_, $_ } }, // ASTC12x10
        texture_format_info { VK_FORMAT_ASTC_12x12_UNORM_BLOCK,    VK_FORMAT_ASTC_12x12_UNORM_BLOCK,   VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_12x12_SRGB_BLOCK,    { $_, $_, $_, $_ } }, // ASTC12x12
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // Unknown
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R1
        texture_format_info { VK_FORMAT_R8_UNORM,                  VK_FORMAT_R8_UNORM,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $0, $0, $0, $R } }, // A8
        texture_format_info { VK_FORMAT_R8_UNORM,                  VK_FORMAT_R8_UNORM,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_R8_SRGB,                  { $_, $_, $_, $_ } }, // R8
        texture_format_info { VK_FORMAT_R8_SINT,                   VK_FORMAT_R8_SINT,                  VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R8I
        texture_format_info { VK_FORMAT_R8_UINT,                   VK_FORMAT_R8_UINT,                  VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R8U
        texture_format_info { VK_FORMAT_R8_SNORM,                  VK_FORMAT_R8_SNORM,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R8S
        texture_format_info { VK_FORMAT_R16_UNORM,                 VK_FORMAT_R16_UNORM,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16
        texture_format_info { VK_FORMAT_R16_SINT,                  VK_FORMAT_R16_SINT,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16I
        texture_format_info { VK_FORMAT_R16_UINT,                  VK_FORMAT_R16_UINT,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16U
        texture_format_info { VK_FORMAT_R16_SFLOAT,                VK_FORMAT_R16_SFLOAT,               VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16F
        texture_format_info { VK_FORMAT_R16_SNORM,                 VK_FORMAT_R16_SNORM,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16S
        texture_format_info { VK_FORMAT_R32_SINT,                  VK_FORMAT_R32_SINT,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R32I
        texture_format_info { VK_FORMAT_R32_UINT,                  VK_FORMAT_R32_UINT,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R32U
        texture_format_info { VK_FORMAT_R32_SFLOAT,                VK_FORMAT_R32_SFLOAT,               VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R32F
        texture_format_info { VK_FORMAT_R8G8_UNORM,                VK_FORMAT_R8G8_UNORM,               VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8_SRGB,                { $_, $_, $_, $_ } }, // RG8
        texture_format_info { VK_FORMAT_R8G8_SINT,                 VK_FORMAT_R8G8_SINT,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG8I
        texture_format_info { VK_FORMAT_R8G8_UINT,                 VK_FORMAT_R8G8_UINT,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG8U
        texture_format_info { VK_FORMAT_R8G8_SNORM,                VK_FORMAT_R8G8_SNORM,               VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG8S
        texture_format_info { VK_FORMAT_R16G16_UNORM,              VK_FORMAT_R16G16_UNORM,             VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16
        texture_format_info { VK_FORMAT_R16G16_SINT,               VK_FORMAT_R16G16_SINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16I
        texture_format_info { VK_FORMAT_R16G16_UINT,               VK_FORMAT_R16G16_UINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16U
        texture_format_info { VK_FORMAT_R16G16_SFLOAT,             VK_FORMAT_R16G16_SFLOAT,            VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16F
        texture_format_info { VK_FORMAT_R16G16_SNORM,              VK_FORMAT_R16G16_SNORM,             VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16S
        texture_format_info { VK_FORMAT_R32G32_SINT,               VK_FORMAT_R32G32_SINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG32I
        texture_format_info { VK_FORMAT_R32G32_UINT,               VK_FORMAT_R32G32_UINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG32U
        texture_format_info { VK_FORMAT_R32G32_SFLOAT,             VK_FORMAT_R32G32_SFLOAT,            VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG32F
        texture_format_info { VK_FORMAT_R8G8B8_UNORM,              VK_FORMAT_R8G8B8_UNORM,             VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8_SRGB,              { $_, $_, $_, $_ } }, // RGB8
        texture_format_info { VK_FORMAT_R8G8B8_SINT,               VK_FORMAT_R8G8B8_SINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8_SRGB,              { $_, $_, $_, $_ } }, // RGB8I
        texture_format_info { VK_FORMAT_R8G8B8_UINT,               VK_FORMAT_R8G8B8_UINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8_SRGB,              { $_, $_, $_, $_ } }, // RGB8U
        texture_format_info { VK_FORMAT_R8G8B8_SNORM,              VK_FORMAT_R8G8B8_SNORM,             VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGB8S
        texture_format_info { VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,    VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,   VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGB9E5F
        texture_format_info { VK_FORMAT_B8G8R8A8_UNORM,            VK_FORMAT_B8G8R8A8_UNORM,           VK_FORMAT_UNDEFINED,           VK_FORMAT_B8G8R8A8_SRGB,            { $_, $_, $_, $_ } }, // BGRA8
        texture_format_info { VK_FORMAT_R8G8B8A8_UNORM,            VK_FORMAT_R8G8B8A8_UNORM,           VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8A8_SRGB,            { $_, $_, $_, $_ } }, // RGBA8
        texture_format_info { VK_FORMAT_R8G8B8A8_SINT,             VK_FORMAT_R8G8B8A8_SINT,            VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8A8_SRGB,            { $_, $_, $_, $_ } }, // RGBA8I
        texture_format_info { VK_FORMAT_R8G8B8A8_UINT,             VK_FORMAT_R8G8B8A8_UINT,            VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8A8_SRGB,            { $_, $_, $_, $_ } }, // RGBA8U
        texture_format_info { VK_FORMAT_R8G8B8A8_SNORM,            VK_FORMAT_R8G8B8A8_SNORM,           VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA8S
        texture_format_info { VK_FORMAT_R16G16B16A16_UNORM,        VK_FORMAT_R16G16B16A16_UNORM,       VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16
        texture_format_info { VK_FORMAT_R16G16B16A16_SINT,         VK_FORMAT_R16G16B16A16_SINT,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16I
        texture_format_info { VK_FORMAT_R16G16B16A16_UINT,         VK_FORMAT_R16G16B16A16_UINT,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16U
        texture_format_info { VK_FORMAT_R16G16B16A16_SFLOAT,       VK_FORMAT_R16G16B16A16_SFLOAT,      VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16F
        texture_format_info { VK_FORMAT_R16G16B16A16_SNORM,        VK_FORMAT_R16G16B16A16_SNORM,       VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16S
        texture_format_info { VK_FORMAT_R32G32B32A32_SINT,         VK_FORMAT_R32G32B32A32_SINT,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA32I
        texture_format_info { VK_FORMAT_R32G32B32A32_UINT,         VK_FORMAT_R32G32B32A32_UINT,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA32U
        texture_format_info { VK_FORMAT_R32G32B32A32_SFLOAT,       VK_FORMAT_R32G32B32A32_SFLOAT,      VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA32F
        texture_format_info { VK_FORMAT_R5G6B5_UNORM_PACK16,       VK_FORMAT_R5G6B5_UNORM_PACK16,      VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // B5G6R5
        texture_format_info { VK_FORMAT_B5G6R5_UNORM_PACK16,       VK_FORMAT_B5G6R5_UNORM_PACK16,      VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R5G6B5
        texture_format_info { VK_FORMAT_B4G4R4A4_UNORM_PACK16,     VK_FORMAT_B4G4R4A4_UNORM_PACK16,    VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $G, $R, $A, $B } }, // BGRA4
        texture_format_info { VK_FORMAT_R4G4B4A4_UNORM_PACK16,     VK_FORMAT_R4G4B4A4_UNORM_PACK16,    VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $A, $B, $G, $R } }, // RGBA4
        texture_format_info { VK_FORMAT_A1R5G5B5_UNORM_PACK16,     VK_FORMAT_A1R5G5B5_UNORM_PACK16,    VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // BGR5A1
        texture_format_info { VK_FORMAT_A1R5G5B5_UNORM_PACK16,     VK_FORMAT_A1R5G5B5_UNORM_PACK16,    VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $B, $G, $R, $A } }, // RGB5A1
        texture_format_info { VK_FORMAT_A2R10G10B10_UNORM_PACK32,  VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $B, $G, $R, $A } }, // RGB10A2
        texture_format_info { VK_FORMAT_B10G11R11_UFLOAT_PACK32,   VK_FORMAT_B10G11R11_UFLOAT_PACK32,  VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG11B10F
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // UnknownDepth
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R16_UNORM,                VK_FORMAT_D16_UNORM,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D16
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT_S8_UINT,  VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D24
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT_S8_UINT,  VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D24S8
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT_S8_UINT,  VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D32
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT,          VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D16F
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT,          VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D24F
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT,          VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D32F
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R8_UINT,                  VK_FORMAT_S8_UINT,             VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D0S8
        #undef $_
        #undef $0
        #undef $1
        #undef $R
        #undef $G
        #undef $B
        #undef $A
    };
    static_assert(bimg::TextureFormat::Count == std::size(k_texture_format_map));
}
