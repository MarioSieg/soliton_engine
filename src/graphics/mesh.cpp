// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "mesh.hpp"
#include "vulkancore/context.hpp"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include "material.hpp"

namespace graphics {
	static auto load_primitive(
		std::vector<mesh::vertex>& vertices,
		std::vector<mesh::index>& indices,
		const aiMesh* mesh,
		mesh::primitive& prim_info,
		DirectX::BoundingBox& full_aabb
	) -> void {
		prim_info.vertex_start = vertices.size();
		prim_info.index_start = indices.size();
		for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
			mesh::vertex v {};
			v.position = std::bit_cast<DirectX::XMFLOAT3>(mesh->mVertices[i]);
			if (mesh->HasNormals()) {
				v.normal = std::bit_cast<DirectX::XMFLOAT3>(mesh->mNormals[i]);
			}
			if (mesh->HasTextureCoords(0)) {
				v.uv = DirectX::XMFLOAT2{mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
			}
			if (mesh->HasTangentsAndBitangents()) {
				v.tangent = std::bit_cast<DirectX::XMFLOAT3>(mesh->mTangents[i]);
				v.bitangent = std::bit_cast<DirectX::XMFLOAT3>(mesh->mBitangents[i]);
			}
			vertices.emplace_back(v);
		}
		for (unsigned i = 0; i < mesh->mNumFaces; ++i) {
			const aiFace face = mesh->mFaces[i];
			if (face.mNumIndices == 3) [[likely]] {
				for (unsigned j = 0; j < 3; ++j) {
					indices.emplace_back(face.mIndices[j]);
				}
			}
		}
		prim_info.index_count = indices.size() - prim_info.index_start;
		prim_info.vertex_count = vertices.size() - prim_info.vertex_start;
		const aiAABB& aabb = mesh->mAABB;
		static_assert(sizeof(DirectX::XMFLOAT3) == sizeof(aiVector3D));
		static_assert(alignof(DirectX::XMFLOAT3) == alignof(aiVector3D));
		DirectX::BoundingBox::CreateFromPoints(prim_info.aabb, DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&aabb.mMin)), DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&aabb.mMax)));
		DirectX::BoundingBox::CreateMerged(full_aabb, full_aabb, prim_info.aabb);
	}

	mesh::mesh(std::string&& path) : asset{asset_category::mesh, asset_source::filesystem, std::move(path)} {
		Assimp::Importer importer {};
		const aiScene* scene = importer.ReadFile(get_asset_path().c_str(), k_import_flags);
		if (!scene || !scene->mNumMeshes) [[unlikely]] {
			panic("Failed to load mesh from file '{}': {}", get_asset_path(), importer.GetErrorString());
		}
		const aiNode* node = scene->mRootNode;
		std::vector<const aiMesh*> meshes {};
		meshes.reserve(node->mNumMeshes);
		for (unsigned i = 0; i < node->mNumMeshes; ++i) {
			const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.emplace_back(mesh);
		}
		create_from_assimp(meshes);
	}

    mesh::mesh(const std::span<const aiMesh*> meshes) : asset{asset_category::mesh, asset_source::memory} {
		create_from_assimp(meshes);
    }

    auto mesh::create_from_assimp(const std::span<const aiMesh*> meshes) -> void {
		std::size_t num_vertices = 0, num_indices = 0;
		for (const aiMesh* mesh : meshes) {
			num_vertices += mesh->mNumVertices;
			num_indices += mesh->mNumFaces * 3;
		}
		std::vector<vertex> vertices {};
		std::vector<index> indices {};
		vertices.reserve(num_vertices);
		indices.reserve(num_indices);
		m_primitives.reserve(meshes.size());
		for (const aiMesh* mesh : meshes) {
			primitive prim_info {};
			load_primitive(vertices, indices, mesh, prim_info, m_aabb);
			m_primitives.emplace_back(prim_info);
		}
		m_primitives.shrink_to_fit();
		create_collision_mesh(vertices, indices);
		create_buffers(vertices, indices);
		m_approx_byte_size = sizeof(*this)
			+ m_vertex_buffer.get_size()
			+ m_index_buffer.get_size()
			+ m_primitives.size() * sizeof(primitive);
    }

    auto mesh::create_buffers(const std::span<const vertex> vertices, const std::span<const index> indices) -> void {
    	passert(indices.size() <= std::numeric_limits<index>::max());

    	m_vertex_buffer.create(
			vertices.size() * sizeof(vertices[0]),
			0,
			vk::BufferUsageFlagBits::eVertexBuffer,
			VMA_MEMORY_USAGE_GPU_ONLY,
			0,
			vertices.data()
		);

    	if (indices.size() <= std::numeric_limits<std::uint16_t>::max()) { // 16 bit indices
    		std::vector<std::uint16_t> indices16 {};
    		indices16.reserve(indices.size());
    		for (const index idx : indices) {
    			indices16.emplace_back(static_cast<std::uint16_t>(idx));
    		}
    		m_index_32bit = false;
    		m_index_count = static_cast<std::uint32_t>(indices16.size());
    		m_index_buffer.create(
				indices16.size() * sizeof(std::uint16_t),
				0,
				vk::BufferUsageFlagBits::eIndexBuffer,
				VMA_MEMORY_USAGE_GPU_ONLY,
				0,
				indices16.data()
			);
    	} else {
    		m_index_32bit = true;
    		m_index_count = static_cast<std::uint32_t>(indices.size());
    		m_index_buffer.create(
				indices.size() * sizeof(index),
				0,
				vk::BufferUsageFlagBits::eIndexBuffer,
				VMA_MEMORY_USAGE_GPU_ONLY,
				0,
				indices.data()
			);
    	}
    }

    auto mesh::create_collision_mesh(const std::span<const vertex> vertices, const std::span<const index> indices) -> void {
		for (const vertex& v : vertices) {
			verts.emplace_back(std::bit_cast<JPH::Float3>(v.position));
		}
		triangles.reserve(indices.size() / 3);
		for (std::size_t i = 0; i < indices.size(); i += 3) {
			JPH::IndexedTriangle tri {};
			tri.mIdx[0] = indices[i];
			tri.mIdx[1] = indices[i + 1];
			tri.mIdx[2] = indices[i + 2];
			triangles.emplace_back(tri);
		}
	}
}
