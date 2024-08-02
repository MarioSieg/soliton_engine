// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/core.hpp"

namespace lu::graphics {
    struct vertex final {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT2 uv;
        DirectX::XMFLOAT3 tangent;
        DirectX::XMFLOAT3 bitangent;
    };
    static_assert(sizeof(vertex) % 4 == 0);

    using index = std::uint32_t; // Meshes generally use 32-bit indices, 16-bit indices are used internally, when possible

    struct primitive final {
        std::uint32_t index_start = 0;
        std::uint32_t index_count = 0;
        std::uint32_t vertex_start = 0;
        std::uint32_t vertex_count = 0;
        DirectX::BoundingBox aabb {};
    };
}
