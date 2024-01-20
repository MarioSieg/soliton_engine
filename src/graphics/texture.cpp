// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "texture.hpp"
#include "subsystem.hpp"

#include <fstream>

#include <bimg/bimg.h>
#include <bimg/decode.h>
#include <bimg/encode.h>
#include <bx/allocator.h>

texture::texture(std::string&& path, bool gen_mips) {
    log_info("Loading texture from '{}'", path);
    std::ifstream file {path, std::ios::binary};
    passert(file.is_open() && file.good());
    file.unsetf(std::ios::skipws);
    file.seekg(0, std::ios::end);
    const std::streamsize size = file.tellg();
    passert(size > 0 && size <= std::numeric_limits<std::uint32_t>::max());
    file.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> mem {};
    mem.resize(size);
    file.read(reinterpret_cast<char*>(mem.data()), size);
    const bool is_dds = std::filesystem::path { path }.extension().string() == ".dds";
    graphics::allocator alloc {};
    bimg::ImageContainer* image = bimg::imageParse(dynamic_cast<bx::AllocatorI*>(&alloc), mem.data(), static_cast<std::uint32_t>(mem.size()));
    passert(image != nullptr);
    if (!is_dds && gen_mips && !image->m_cubeMap && image->m_numMips <= 1) {
        auto* const with_mips = bimg::imageGenerateMips(dynamic_cast<bx::AllocatorI*>(&alloc), *image);
        if (with_mips) [[likely]] {
            bimg::imageFree(image);
            image = with_mips;
        } else {
            log_warn("Failed to generate mipmaps for texture: '{}'", path);
        }
    }
    const std::span texels {
        static_cast<const std::uint8_t*>(image->m_data),
        static_cast<const std::uint8_t*>(image->m_data) + image->m_size
    };
    passert(bgfx::isTextureValid(image->m_depth, image->m_cubeMap, image->m_numLayers, static_cast<bgfx::TextureFormat::Enum>(image->m_format), BGFX_TEXTURE_NONE));
    const bgfx::Memory* const texel_gpu_buf = bgfx::alloc(texels.size());
    std::memcpy(texel_gpu_buf->data, texels.data(), texels.size());
    bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
    if (image->m_cubeMap) {
        assert(image->m_width == image->m_height);
        handle = bgfx::createTextureCube(image->m_width, false, image->m_numLayers, static_cast<bgfx::TextureFormat::Enum>(image->m_format), BGFX_TEXTURE_NONE, texel_gpu_buf);
    } else {
        handle = bgfx::createTexture2D
        (
            image->m_width,
            image->m_height,
            image->m_numMips > 1,
            image->m_numLayers,
            static_cast<bgfx::TextureFormat::Enum>(image->m_format),
            BGFX_TEXTURE_NONE,
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
    flags = BGFX_TEXTURE_NONE;
    num_texels = mem.size();
    texel_buffer = ::handle{handle};
    bimg::imageFree(image);
}
