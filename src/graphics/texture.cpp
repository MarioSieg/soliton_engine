// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "texture.hpp"
#include "subsystem.hpp"

#include <fstream>

#include <bimg/bimg.h>
#include <bimg/decode.h>
#include <bimg/encode.h>
#include <bx/allocator.h>

texture::texture(std::string&& path, bool gen_mips, std::uint64_t flags) {
    static constinit std::atomic_unsigned_lock_free num;
    log_info("Loading texture #{} from '{}'", num.fetch_add(1, std::memory_order_relaxed), path);
    bimg::ImageContainer* image = nullptr;
    static graphics::allocator alloc {};
    {
        std::vector<std::uint8_t> mem {};
        {
            std::ifstream file {path, std::ios::binary};
            passert(file.is_open() && file.good());
            file.unsetf(std::ios::skipws);
            file.seekg(0, std::ios::end);
            const std::streamsize size = file.tellg();
            passert(size > 0 && size <= std::numeric_limits<std::uint32_t>::max());
            file.seekg(0, std::ios::beg);
            mem.resize(size);
            file.read(reinterpret_cast<char*>(mem.data()), size);
        }
        image = bimg::imageParse(&alloc, mem.data(), static_cast<std::uint32_t>(mem.size()));
        passert(image != nullptr);
    }

    const bool is_dds = std::filesystem::path { path }.extension().string() == ".dds";
    if (!is_dds && gen_mips && !image->m_cubeMap && image->m_numMips <= 1) {
        if (image->m_format != bimg::TextureFormat::RGBA8 && image->m_format != bimg::TextureFormat::RGBA32F) { // Mipgen is only supported for these
            auto* const transcoded = bimg::imageConvert(&alloc, bimg::TextureFormat::RGBA8, *image, false);
            if (transcoded) [[likely]] {
                bimg::imageFree(image);
                image = transcoded;
            } else {
                log_warn("Failed to transcode texture: '{}' with format: {} to format: {}", path, bimg::getName(image->m_format), bimg::getName(bimg::TextureFormat::RGBA8));
            }
        }
        auto* const with_mips = bimg::imageGenerateMips(&alloc, *image);
        if (with_mips) [[likely]] {
            bimg::imageFree(image);
            image = with_mips;
        } else {
            log_warn("Failed to generate mipmaps for texture: '{}' with format: {}", path, bimg::getName(image->m_format));
        }
    }
    if (!bgfx::isTextureValid(image->m_depth, image->m_cubeMap, image->m_numLayers, static_cast<bgfx::TextureFormat::Enum>(image->m_format), flags)) {
        log_warn("SRGB sampling not supported for texture: '{}' with format: {}", path, bimg::getName(image->m_format));
        flags &= ~BGFX_TEXTURE_SRGB;
    }
    passert(bgfx::isTextureValid(image->m_depth, image->m_cubeMap, image->m_numLayers, static_cast<bgfx::TextureFormat::Enum>(image->m_format), flags));
    const bgfx::Memory* const texel_gpu_buf = bgfx::makeRef(image->m_data, image->m_size, +[]([[maybe_unused]] void* p, void* usr) {
        bimg::imageFree(static_cast<bimg::ImageContainer*>(usr));
    }, image);
    bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
    if (image->m_cubeMap) {
        passert(image->m_width == image->m_height);
        handle = bgfx::createTextureCube(
            image->m_width,
            image->m_numMips > 1,
            image->m_numLayers,
            static_cast<bgfx::TextureFormat::Enum>(image->m_format),
            flags,
            texel_gpu_buf
        );
    } else {
        handle = bgfx::createTexture2D(
            image->m_width,
            image->m_height,
            image->m_numMips > 1,
            image->m_numLayers,
            static_cast<bgfx::TextureFormat::Enum>(image->m_format),
            flags,
            texel_gpu_buf
        );
    }
    passert(bgfx::isValid(handle));
    file_path = std::move(path);
    width = image->m_width;
    height = image->m_height;
    depth = image->m_depth;
    layers = image->m_numLayers;
    format = static_cast<bgfx::TextureFormat::Enum>(image->m_format);
    num_mips = image->m_numMips;
    is_cubemap = image->m_cubeMap;
    num_texels = image->m_size;
    this->handle = ::handle{handle};
    this->flags = flags;
}
