// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "prelude.hpp"

class texture final : public no_copy, public no_move {
public:
    std::string file_path {};
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint16_t depth = 0;
    std::uint16_t layers = 0;
    bgfx::TextureFormat::Enum format = bgfx::TextureFormat::RGB8;
    std::uint8_t num_mips = 0;
    bool is_cubemap = false;
    std::uint64_t flags = 0;
    std::size_t num_texels = 0;
    handle<bgfx::TextureHandle> handle {};

    explicit texture(std::string&& path, bool gen_mips, std::uint64_t flags = BGFX_TEXTURE_NONE);
};
