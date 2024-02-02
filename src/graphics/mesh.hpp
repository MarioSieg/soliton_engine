// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/buffer.hpp"
#include "../assetmgr/assetmgr.hpp"

#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <tiny_gltf.h>

namespace graphics {
    class material;

    class mesh final : public asset {
    public:
        struct vertex final {
            DirectX::XMFLOAT3 position;
            DirectX::XMFLOAT3 normal;
            DirectX::XMFLOAT2 uv;
            DirectX::XMFLOAT3 tangent;
            DirectX::XMFLOAT3 bitangent;
        };

        using index = std::uint32_t;

        struct primitive final {
            std::uint32_t index_start = 0;
            std::uint32_t index_count = 0;
            std::uint32_t vertex_start = 0;
            std::uint32_t vertex_count = 0;
            std::int32_t src_material_index = 0;
            std::uint32_t dst_material_index = 0;
            DirectX::BoundingBox aabb {};
        };

        explicit mesh(const std::string& path);
        mesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh);
        ~mesh() override = default;

        [[nodiscard]] auto get_primitives() const noexcept -> std::span<const primitive> { return m_primitives; }
        [[nodiscard]] auto get_aabb() const noexcept -> const DirectX::BoundingBox& { return m_aabb; }
        [[nodiscard]] auto get_vertex_buffer() const noexcept -> const vkb::buffer& { return m_vertex_buffer; }
        [[nodiscard]] auto get_index_buffer() const noexcept -> const vkb::buffer& { return m_index_buffer; }
        [[nodiscard]] auto get_index_count() const noexcept -> std::uint32_t { return m_index_count; }
        [[nodiscard]] auto is_index_32bit() const noexcept -> bool { return m_index_32bit; }

    private:
        auto create_from_gltf(const tinygltf::Model& mode, const tinygltf::Mesh& mesh) -> void;
        auto create_buffers(std::span<const vertex> vertices, std::span<const index> indices) -> void;
        auto recompute_bounds(std::span<const vertex> vertices) -> void;

        vkb::buffer m_vertex_buffer {};
        vkb::buffer m_index_buffer {};
        std::uint32_t m_index_count = 0;
        bool m_index_32bit = false;
        std::vector<primitive> m_primitives {};
        DirectX::BoundingBox m_aabb {};
    };
}
