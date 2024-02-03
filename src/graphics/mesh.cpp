// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "mesh.hpp"
#include "vulkancore/context.hpp"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "material.hpp"

namespace graphics {
	static auto load_primitive(
		std::vector<mesh::vertex>& vertices,
		std::vector<mesh::index>& indices,
		const aiMesh* mesh,
		mesh::primitive& prim_info
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
	}

	mesh::mesh(std::string&& path) : asset{asset_category::mesh, asset_source::filesystem, std::move(path)} {
		Assimp::Importer importer {};
		unsigned k_import_flags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded;
		k_import_flags &= ~(aiProcess_ValidateDataStructure | aiProcess_SplitLargeMeshes);
		const aiScene* scene = importer.ReadFile(get_asset_path().c_str(), k_import_flags);
		if (!scene || !scene->mNumMeshes) [[unlikely]] {
			panic("Failed to load mesh from file '{}': {}", get_asset_path(), importer.GetErrorString());
		}
		std::vector<const aiMesh*> meshes {};
		meshes.reserve(scene->mNumMeshes);
		for (std::size_t i = 0; i < scene->mNumMeshes; ++i) {
			meshes.emplace_back(scene->mMeshes[i]);
		}
		create_from_assimp(meshes);
	}

    mesh::mesh(const std::span<const aiMesh*> meshes) : asset{asset_category::mesh, asset_source::memory} {
		create_from_assimp(meshes);
    }

    auto mesh::recompute_bounds(const std::span<const vertex> vertices) -> void {
    	for (primitive& prim : m_primitives) {
    		DirectX::XMVECTOR min = DirectX::XMVectorReplicate(std::numeric_limits<float>::max());
    		DirectX::XMVECTOR max = DirectX::XMVectorReplicate(std::numeric_limits<float>::lowest());
    		for (std::size_t i = prim.vertex_start; i < prim.vertex_count; ++i) {
    			const vertex& v = vertices[i];
    			DirectX::XMVECTOR pos = XMLoadFloat3(&v.position);
    			min = DirectX::XMVectorMin(min, pos);
    			max = DirectX::XMVectorMax(max, pos);
    		}
    		DirectX::BoundingBox::CreateFromPoints(prim.aabb, min, max);
    		m_aabb.CreateMerged(m_aabb, m_aabb, prim.aabb);
    	}
    }

    auto mesh::create_from_assimp(const std::span<const aiMesh*> meshes) -> void {
		m_primitives.reserve(meshes.size());
		std::size_t num_vertices = 0, num_indices = 0;
		for (const aiMesh* mesh : meshes) {
			num_vertices += mesh->mNumVertices;
			num_indices += mesh->mNumFaces * 3;
		}
		std::vector<vertex> vertices {};
		std::vector<index> indices {};
		vertices.reserve(num_vertices);
		indices.reserve(num_indices);
		for (const aiMesh* mesh : meshes) {
			primitive prim_info {};
			load_primitive(vertices, indices, mesh, prim_info);
			m_primitives.emplace_back(prim_info);
		}
		m_primitives.shrink_to_fit();
		recompute_bounds(vertices);
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
				indices16.size() * sizeof(indices16[0]),
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
				indices.size() * sizeof(indices[0]),
				0,
				vk::BufferUsageFlagBits::eIndexBuffer,
				VMA_MEMORY_USAGE_GPU_ONLY,
				0,
				indices.data()
			);
    	}
    }
}
