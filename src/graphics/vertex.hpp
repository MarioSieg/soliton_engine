// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"

namespace lu::graphics {
    struct vertex final {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT2 uv;
        DirectX::XMFLOAT3 tangent;
        DirectX::XMFLOAT3 bitangent;
    };
    static_assert(sizeof(vertex) % 4 == 0);

    constexpr eastl::array<vk::VertexInputBindingDescription, 1> k_vertex_binding_desc {
        vk::VertexInputBindingDescription {
            .binding = 0,
            .stride = sizeof(vertex),
            .inputRate = vk::VertexInputRate::eVertex
        }
    };

    constexpr eastl::array<vk::VertexInputAttributeDescription, 5> k_vertex_attrib_desc {
        vk::VertexInputAttributeDescription {
            .location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(vertex, position)
        },
        vk::VertexInputAttributeDescription {
            .location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(vertex, normal)
        },
        vk::VertexInputAttributeDescription {
            .location = 2,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = offsetof(vertex, uv)
        },
        vk::VertexInputAttributeDescription {
            .location = 3,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(vertex, tangent)
        },
        vk::VertexInputAttributeDescription {
            .location = 4,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(vertex, bitangent)
        }
    };

    using index = std::uint32_t; // Meshes generally use 32-bit indices, 16-bit indices are used internally, when possible

    struct primitive final {
        std::uint32_t index_start = 0;
        std::uint32_t index_count = 0;
        std::uint32_t vertex_start = 0;
        std::uint32_t vertex_count = 0;
        DirectX::BoundingBox aabb {};
    };
}
