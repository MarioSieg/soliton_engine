// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/buffer.hpp"
#include "../assetmgr/assetmgr.hpp"
#include "vertex.hpp"

#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

namespace lu::graphics {
    class material;

    class mesh final : public assetmgr::asset {
    public:
        explicit mesh(eastl::string&& path);
        explicit mesh(eastl::span<const aiMesh*> meshes);
        ~mesh() override = default;

        [[nodiscard]] auto get_primitives() const noexcept -> eastl::span<const primitive> { return m_primitives; }
        [[nodiscard]] auto get_aabb() const noexcept -> const DirectX::BoundingBox& { return m_aabb; }
        [[nodiscard]] auto get_vertex_buffer() const noexcept -> const vkb::buffer& { return m_vertex_buffer; }
        [[nodiscard]] auto get_index_buffer() const noexcept -> const vkb::buffer& { return m_index_buffer; }
        [[nodiscard]] auto get_vertex_count() const noexcept -> std::uint32_t { return m_vertex_count; }
        [[nodiscard]] auto get_index_count() const noexcept -> std::uint32_t { return m_index_count; }
        [[nodiscard]] auto is_index_32bit() const noexcept -> bool { return m_index_32bit; }
        [[nodiscard]] auto get_collision_mesh() const noexcept -> const JPH::MeshShape* { return m_collision_mesh; }

        static constexpr std::uint32_t k_import_flags = []() noexcept -> std::uint32_t { // TODO use flags from lua
            std::uint32_t k_import_flags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded;
            k_import_flags |= aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_GenBoundingBoxes;
            k_import_flags |= aiProcess_FixInfacingNormals;
            return k_import_flags;
        }();

        std::vector<JPH::Float3, JPH::STLAllocator<JPH::Float3>> verts {};
        std::vector<JPH::IndexedTriangle, JPH::STLAllocator<JPH::IndexedTriangle>> triangles {};

    private:
        auto create_from_assimp(eastl::span<const aiMesh*> meshes) -> void;
        auto create_buffers(eastl::span<const vertex> vertices, eastl::span<const index> indices) -> void;
        auto create_collision_mesh(eastl::span<const vertex> vertices, eastl::span<const index> indices) -> void;

        vkb::buffer m_vertex_buffer {};
        vkb::buffer m_index_buffer {};
        std::uint32_t m_vertex_count = 0;
        std::uint32_t m_index_count = 0;
        bool m_index_32bit = false;
        eastl::vector<primitive> m_primitives {};
        DirectX::BoundingBox m_aabb {};
        JPH::MeshShape* m_collision_mesh {};
    };
}
