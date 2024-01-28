// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../scene/scene.hpp"
#include "../vulkancore/buffer.hpp"
#include "../gltf/tiny_gltf.h"

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
        mesh(std::span<const vertex> vertices, std::span<const index> indices);
        mesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, FXMMATRIX transform = XMMatrixIdentity());
        ~mesh();

        auto draw(vk::CommandBuffer cmd) -> void;

        [[nodiscard]] auto get_primitives() const noexcept -> std::span<const primitive> { return m_primitives; }
        [[nodiscard]] auto get_aabb() const noexcept -> const BoundingBox& { return aabb; }
        [[nodiscard]] auto get_vertex_buffer() const noexcept -> const vkb::buffer& { return m_vertex_buffer; }
        [[nodiscard]] auto get_index_buffer() const noexcept -> const vkb::buffer& { return m_index_buffer; }
        [[nodiscard]] auto get_index_count() const noexcept -> std::uint32_t { return m_index_count; }
        [[nodiscard]] auto is_index_32bit() const noexcept -> bool { return m_index_32bit; }

    private:
        auto create_buffers(std::span<const vertex> vertices, std::span<const index> indices) -> void;
        auto load_mesh_from_gltf(const tinygltf::Model& model, const tinygltf::Mesh& mesh, FXMMATRIX transform) -> void;

        vkb::buffer m_vertex_buffer {};
        vkb::buffer m_index_buffer {};
        std::uint32_t m_index_count = 0;
        bool m_index_32bit = false;
        std::vector<primitive> m_primitives {};
        BoundingBox aabb {};
    };
}
