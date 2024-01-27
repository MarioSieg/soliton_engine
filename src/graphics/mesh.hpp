// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../scene/scene.hpp"
#include "../vulkancore/buffer.hpp"

namespace graphics {
    struct vertex final {
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT2 uv;
        XMFLOAT3 tangent;
        XMFLOAT3 bitangent;
    };

    class mesh final : public no_copy, public no_move {
    public:
        explicit mesh(const std::string& path);
        ~mesh();

    private:
        vkb::buffer m_vertex_buffer {};
        vkb::buffer m_index_buffer {};
        std::uint32_t m_index_count = 0;
    };
}
