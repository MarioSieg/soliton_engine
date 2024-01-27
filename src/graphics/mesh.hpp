// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../scene/scene.hpp"
#include "../vulkancore/buffer.hpp"

namespace graphics {
    class mesh final : public no_copy, public no_move {
    public:
        struct vertex final {
            XMFLOAT3 position;
            XMFLOAT3 normal;
            XMFLOAT2 uv;
            XMFLOAT3 tangent;
            XMFLOAT3 bitangent;
        };

        using index = std::uint32_t;

        struct primitive final {
            std::uint32_t first_index = 0;
            std::uint32_t index_count = 0;
            std::uint32_t first_vertex = 0;
            std::uint32_t vertex_count = 0;
            BoundingBox aabb {};
        };

        explicit mesh(const std::string& path);
        ~mesh();

        auto draw(vk::CommandBuffer cmd) -> void;

        [[nodiscard]] auto get_primitives() const noexcept -> std::span<const primitive> { return m_primitives; }
        [[nodiscard]] auto get_aabb() const noexcept -> const BoundingBox& { return aabb; }

    private:
        vkb::buffer m_vertex_buffer {};
        vkb::buffer m_index_buffer {};
        std::uint32_t m_index_count = 0;
        std::vector<primitive> m_primitives {};
        BoundingBox aabb {};
    };
}
