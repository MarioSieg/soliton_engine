// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "mesh.hpp"
#include "vulkancore/context.hpp"

#include <assimp/scene.h>
#include <assimp/cimport.h>

#include <tiny_gltf.h>
#include <assimp/postprocess.h>

#include "material.hpp"

namespace graphics {
	[[nodiscard]] static auto load_primitive(
		std::vector<mesh::vertex>& vertices,
		std::vector<mesh::index>& indices,
		const tinygltf::Primitive& prim,
		const tinygltf::Model& model,
		mesh::primitive& prim_info
	) -> bool;

	mesh::mesh(const std::string& path) : asset{asset_category::mesh, asset_source::filesystem, std::string {path}} {
		const aiScene* scene = aiImportFile(path.c_str(), aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded | aiProcess_CalcTangentSpace);
		passert(scene != nullptr);
		passert(scene->mNumMeshes > 0);
		aiMesh* mesh = scene->mMeshes[0];
		std::vector<vertex> vertices {};
		std::vector<index> indices {};
		m_primitives.reserve(scene->mNumMeshes);
		for (std::size_t i = 0; i < scene->mNumMeshes; ++i) {
			primitive prim_info {};
			aiMesh* mesh = scene->mMeshes[i];
			for (std::size_t j = 0; j < mesh->mNumVertices; ++j) {
				vertex v {};
				v.position = DirectX::XMFLOAT3{mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z};
				v.normal = DirectX::XMFLOAT3{mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z};
				if (mesh->HasTextureCoords(0)) {
					v.uv = DirectX::XMFLOAT2{mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y};
				}
				if (mesh->HasTangentsAndBitangents()) {
					v.tangent = DirectX::XMFLOAT3{mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z};
					v.bitangent = DirectX::XMFLOAT3{mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z};
				}
				vertices.emplace_back(v);
			}
			for (std::size_t j = 0; j < mesh->mNumFaces; ++j) {
				aiFace face = mesh->mFaces[j];
				for (std::size_t k = 0; k < face.mNumIndices; ++k) {
					indices.emplace_back(face.mIndices[k]);
				}
			}
			prim_info.index_start = indices.size() - mesh->mNumFaces * 3;
			prim_info.index_count = mesh->mNumFaces * 3;
			prim_info.vertex_start = vertices.size() - mesh->mNumVertices;
			prim_info.vertex_count = mesh->mNumVertices;
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

    mesh::mesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh) : asset{asset_category::mesh, asset_source::memory} {
		create_from_gltf(model, mesh);
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

    auto mesh::create_from_gltf(const tinygltf::Model& model, const tinygltf::Mesh& mesh) -> void {
		std::vector<vertex> vertices {};
		std::vector<index> indices {};
		m_primitives.reserve(mesh.primitives.size());
		for (const tinygltf::Primitive& prim : mesh.primitives) {
			primitive prim_info {};
			if (load_primitive(vertices, indices, prim, model, prim_info)) {
				m_primitives.emplace_back(prim_info);
			}
		}
		m_primitives.shrink_to_fit();
		recompute_bounds(vertices);
		create_buffers(vertices, indices);
		m_approx_byte_size = sizeof(*this)
			+ m_vertex_buffer.get_size()
			+ m_index_buffer.get_size()
			+ m_primitives.size() * sizeof(primitive);
    }

    auto mesh::create_buffers(std::span<const vertex> vertices, std::span<const index> indices) -> void {
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

    static auto load_primitive(
		std::vector<mesh::vertex>& vertices,
		std::vector<mesh::index>& indices,
		const tinygltf::Primitive& prim,
		const tinygltf::Model& model,
		mesh::primitive& prim_info
	) -> bool {
		const std::size_t vertex_start = vertices.size();
        const std::size_t index_start = indices.size();
        std::size_t num_vertices = 0;
        std::size_t num_indices = 0;

        if (prim.indices < 0) [[unlikely]] {
            log_error("GLTF import: Mesh has no indices");
            return false;
        }
		// Position attribute is required
		if (!prim.attributes.contains("POSITION")) [[unlikely]] {
			log_error("GLTF import: Mesh has no position attribute");
			return false;
		}

    	if (prim.material > -1) {
    		prim_info.src_material_index = prim.material;
    	}

    	// Indices
	    {
			const tinygltf::Accessor& accessor = model.accessors[prim.indices];
			num_indices = accessor.count;
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
			switch (accessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
					std::unique_ptr<std::uint32_t[]> buf {new std::uint32_t[accessor.count]};
					std::memcpy(buf.get(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
					for (std::size_t i = 0; i < accessor.count; ++i) {
						indices.emplace_back(buf[i] + vertex_start);
					}
				} break;
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
					std::unique_ptr<std::uint16_t[]> buf {new std::uint16_t[accessor.count]};
					std::memcpy(buf.get(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
					for (std::size_t i = 0; i < accessor.count; ++i) {
						indices.emplace_back(buf[i] + vertex_start);
					}
				} break;
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
					std::unique_ptr<std::uint8_t[]> buf {new std::uint8_t[accessor.count]};
					std::memcpy(buf.get(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
					for (std::size_t i = 0; i < accessor.count; ++i) {
						indices.emplace_back(buf[i] + vertex_start);
					}
				} break;
				default:
					log_error("GLTF import: Index component type {} not supported", accessor.componentType);
			}
	    }

        // Vertices
		{
			const tinygltf::Accessor& pos_accessor = model.accessors[prim.attributes.find("POSITION")->second];
			num_vertices = pos_accessor.count;
			const tinygltf::BufferView& pos_view = model.bufferViews[pos_accessor.bufferView];

            const float* buffer_pos = nullptr;
            const float* buffer_normals = nullptr;
            const float* buffer_tex_coords = nullptr;
            const float* buffer_tangents = nullptr;
			buffer_pos = reinterpret_cast<const float*>(&model.buffers[pos_view.buffer].data[pos_accessor.byteOffset + pos_view.byteOffset]);

			if (prim.attributes.contains("NORMAL")) {
				const tinygltf::Accessor& norm_accessor = model.accessors[prim.attributes.find("NORMAL")->second];
				const tinygltf::BufferView& norm_view = model.bufferViews[norm_accessor.bufferView];
				buffer_normals = reinterpret_cast<const float*>(&(model.buffers[norm_view.buffer].data[norm_accessor.byteOffset + norm_view.byteOffset]));
			}
			if (prim.attributes.contains("TEXCOORD_0")) {
				const tinygltf::Accessor& uv_accessor = model.accessors[prim.attributes.find("TEXCOORD_0")->second];
				const tinygltf::BufferView& uv_view = model.bufferViews[uv_accessor.bufferView];
				buffer_tex_coords = reinterpret_cast<const float*>(&(model.buffers[uv_view.buffer].data[uv_accessor.byteOffset + uv_view.byteOffset]));
			}
			if (prim.attributes.contains("TANGENT")) {
				const tinygltf::Accessor& tangent_accessor = model.accessors[prim.attributes.find("TANGENT")->second];
				const tinygltf::BufferView& tangent_view = model.bufferViews[tangent_accessor.bufferView];
				buffer_tangents = reinterpret_cast<const float*>(&(model.buffers[tangent_view.buffer].data[tangent_accessor.byteOffset + tangent_view.byteOffset]));
			}

            num_vertices = pos_accessor.count;
            for (std::size_t i = 0; i < num_vertices; ++i) {
            	mesh::vertex vv {
            		.position = *reinterpret_cast<const DirectX::XMFLOAT3*>(buffer_pos + i * 3),
            		.normal = buffer_normals ? *reinterpret_cast<const DirectX::XMFLOAT3*>(buffer_normals + i * 3) : DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},
            		.uv = buffer_tex_coords ? *reinterpret_cast<const DirectX::XMFLOAT2*>(buffer_tex_coords + i * 2) : DirectX::XMFLOAT2{0.0f, 0.0f},
            		.tangent = buffer_tangents ? *reinterpret_cast<const DirectX::XMFLOAT3*>(buffer_tangents + i * 3) : DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f},
            		.bitangent = DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f} // TODO
            	};
            	vertices.emplace_back(vv);
            }
			if (!buffer_tangents) {
				for (std::size_t i = 0; i < indices.size(); i += 3) {
					mesh::vertex& v0 = vertices[indices[i]];
					mesh::vertex& v1 = vertices[indices[i + 1]];
					mesh::vertex& v2 = vertices[indices[i + 2]];

					const DirectX::XMVECTOR pos0 = DirectX::XMLoadFloat3(&v0.position);
					const DirectX::XMVECTOR pos1 = DirectX::XMLoadFloat3(&v1.position);
					const DirectX::XMVECTOR pos2 = DirectX::XMLoadFloat3(&v2.position);
					const DirectX::XMVECTOR uv0 = DirectX::XMLoadFloat2(&v0.uv);
					const DirectX::XMVECTOR uv1 = DirectX::XMLoadFloat2(&v1.uv);
					const DirectX::XMVECTOR uv2 = DirectX::XMLoadFloat2(&v2.uv);

					const DirectX::XMVECTOR edge1 = DirectX::XMVectorSubtract(pos1, pos0);
					const DirectX::XMVECTOR edge2 = DirectX::XMVectorSubtract(pos2, pos0);
					const DirectX::XMVECTOR deltaUV1 = DirectX::XMVectorSubtract(uv1, uv0);
					const DirectX::XMVECTOR deltaUV2 = DirectX::XMVectorSubtract(uv2, uv0);

					const float f = 1.0f / DirectX::XMVectorGetX(DirectX::XMVector2Cross(deltaUV1, deltaUV2));

					DirectX::XMVECTOR tangent = DirectX::XMVectorSet(
						f * (DirectX::XMVectorGetY(deltaUV2) * DirectX::XMVectorGetX(edge1) - DirectX::XMVectorGetY(deltaUV1) * DirectX::XMVectorGetX(edge2)),
						f * (DirectX::XMVectorGetY(deltaUV2) * DirectX::XMVectorGetY(edge1) - DirectX::XMVectorGetY(deltaUV1) * DirectX::XMVectorGetY(edge2)),
						f * (DirectX::XMVectorGetY(deltaUV2) * DirectX::XMVectorGetZ(edge1) - DirectX::XMVectorGetY(deltaUV1) * DirectX::XMVectorGetZ(edge2)),
						0.0f
					);

					DirectX::XMVECTOR bitangent = DirectX::XMVectorSet(
						f * (-DirectX::XMVectorGetX(deltaUV2) * DirectX::XMVectorGetX(edge1) + DirectX::XMVectorGetX(deltaUV1) * DirectX::XMVectorGetX(edge2)),
						f * (-DirectX::XMVectorGetX(deltaUV2) * DirectX::XMVectorGetY(edge1) + DirectX::XMVectorGetX(deltaUV1) * DirectX::XMVectorGetY(edge2)),
						f * (-DirectX::XMVectorGetX(deltaUV2) * DirectX::XMVectorGetZ(edge1) + DirectX::XMVectorGetX(deltaUV1) * DirectX::XMVectorGetZ(edge2)),
						0.0f
					);

					// Adding the computed tangent to each vertex of the triangle
					DirectX::XMStoreFloat3(&v0.tangent, tangent);
					DirectX::XMStoreFloat3(&v1.tangent, tangent);
					DirectX::XMStoreFloat3(&v2.tangent, tangent);
					DirectX::XMStoreFloat3(&v0.bitangent, bitangent);
					DirectX::XMStoreFloat3(&v1.bitangent, bitangent);
					DirectX::XMStoreFloat3(&v2.bitangent, bitangent);
				}

				// Normalize the tangents
				for (auto& vertex : vertices) {
					// Make it orthogonal to the normal
					DirectX::XMVECTOR normal = XMLoadFloat3(&vertex.normal);
					DirectX::XMVECTOR tangent = XMLoadFloat3(&vertex.tangent);

					// Gram-Schmidt orthogonalize
					tangent = DirectX::XMVector3Normalize(tangent - normal * DirectX::XMVector3Dot(normal, tangent));

					// Compute handedness
					DirectX::XMVECTOR handedness = DirectX::XMVector3Dot(DirectX::XMVector3Cross(normal, tangent), XMLoadFloat3(&vertex.bitangent));
					const float sign = DirectX::XMVectorGetX(handedness) < 0.0f ? -1.0f : 1.0f;
					tangent = DirectX::XMVectorMultiply(tangent, DirectX::XMVectorReplicate(sign));

					DirectX::XMStoreFloat3(&vertex.tangent, tangent);
				}
			}
		}

        prim_info.index_start = index_start;
        prim_info.index_count = num_indices;
        prim_info.vertex_start = vertex_start;
        prim_info.vertex_count = num_vertices;
    	return true;
	}
}
