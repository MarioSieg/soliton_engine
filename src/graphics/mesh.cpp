// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "mesh.hpp"
#include "vulkancore/context.hpp"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <filesystem>

#include "material.hpp"
#include "utils/assimp_utils.hpp"

namespace soliton::graphics {
	using namespace DirectX;

	static auto compute_aabb(BoundingBox& aabb, const eastl::span<const vertex> vertices) noexcept -> void {
		XMVECTOR min = XMVectorReplicate(1e10f);
        XMVECTOR max = XMVectorReplicate(-1e10f);
		for (const auto& v : vertices) {
			const auto pos = XMLoadFloat3(&v.position);
			min = XMVectorMin(min, pos);
			max = XMVectorMax(max, pos);
		}
		aabb.CreateFromPoints(aabb, min, max);
	}

	static auto load_primitive(
		eastl::vector<vertex>& vertices,
		eastl::vector<index>& indices,
		const aiMesh* mesh,
		primitive& prim_info,
		BoundingBox& full_aabb
	) -> void {
		prim_info.vertex_start = vertices.size();
		prim_info.index_start = indices.size();
		for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
			vertex v {};
			v.position = eastl::bit_cast<XMFLOAT3>(mesh->mVertices[i]);
			if (mesh->HasNormals()) {
				v.normal = eastl::bit_cast<XMFLOAT3>(mesh->mNormals[i]);
			}
			if (mesh->HasTextureCoords(0)) {
				v.uv = XMFLOAT2{mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
			}
			if (mesh->HasTangentsAndBitangents()) {
				v.tangent = eastl::bit_cast<XMFLOAT3>(mesh->mTangents[i]);
				v.bitangent = eastl::bit_cast<XMFLOAT3>(mesh->mBitangents[i]);
			}
			vertices.emplace_back(v);
		}
        if (mesh->mNumFaces == 0) {
            log_error("Mesh '{}' has no faces - no index buffer will be generated", mesh->mName.C_Str());
        }
		for (unsigned i = 0; i < mesh->mNumFaces; ++i) {
			const aiFace face = mesh->mFaces[i];
			if (face.mNumIndices == 3) {
				for (unsigned j = 0; j < 3; ++j) {
					indices.emplace_back(face.mIndices[j]);
				}
			} else {
                log_error("{}-gon in mesh '{}' - mesh should be triangulated", face.mNumIndices, mesh->mName.C_Str());
                for (unsigned j = 0; j < 3; ++j) {
                    indices.emplace_back(i + j);
                }
            }
		}
		prim_info.index_count = indices.size() - prim_info.index_start;
		prim_info.vertex_count = vertices.size() - prim_info.vertex_start;
		compute_aabb(prim_info.aabb, {vertices.data() + prim_info.vertex_start, prim_info.vertex_count});
	}

	mesh::mesh(eastl::string&& path, const bool create_collider_mesh, const std::uint32_t import_flags) : asset{assetmgr::asset_source::filesystem, std::move(path)} {
        log_info("Loading mesh from file '{}'", get_asset_path());

        Assimp::DefaultLogger::create("", Assimp::Logger::NORMAL);
        Assimp::DefaultLogger::get()->attachStream(new assimp_logger {}, Assimp::Logger::Info | Assimp::Logger::Err | Assimp::Logger::Warn);

        eastl::vector<std::byte> blob {};
        assetmgr::with_primary_accessor_lock([&](assetmgr::asset_accessor& accessor) {
            if (!accessor.load_bin_file(get_asset_path().c_str(), blob)) {
                panic("Failed to load mesh from file '{}'", get_asset_path());
            }
        });

        Assimp::Importer importer {};
        importer.SetIOHandler(new graphics::lunam_assimp_io_system {});
        panic_assert(importer.ValidateFlags(import_flags));
        eastl::string hint {};
        auto a_path {std::filesystem::path{get_asset_path().c_str()}};
        if (a_path.has_extension()) {
            hint = a_path.extension().string().c_str();
        }
        const aiScene* scene = importer.ReadFileFromMemory(blob.data(), blob.size(), import_flags, hint.empty() ? nullptr : hint.c_str());
        if (!scene || !scene->mNumMeshes) {
            panic("Failed to load scene from file '{}': {}", get_asset_path(), importer.GetErrorString());
        }

		const aiNode* node = scene->mRootNode;
        if (!node || node->mNumMeshes == 0) { // search for other nodes with meshes
            eastl::function<auto(const aiNode*) -> const aiNode*> search_for_meshes = [&search_for_meshes](const aiNode* const node) -> const aiNode* {
                for (unsigned i = 0; i < node->mNumChildren; ++i) {
                    auto* local_node = node->mChildren[i];
                    if (local_node && local_node->mNumMeshes > 0) {
                        return local_node;
                    }
                    search_for_meshes(local_node);
                }
                return nullptr;
            };
            node = search_for_meshes(node);
        }

        if (!node || node->mNumMeshes == 0) {
            panic("Scene has no root node and no meshes");
        }

        const aiMesh* mesh = scene->mMeshes[node->mMeshes[0]];
        eastl::span span {&mesh, 1};

		create_from_assimp(span, create_collider_mesh);
	}

    mesh::mesh(const eastl::span<const aiMesh*> meshes, const bool create_collider_mesh) : asset{assetmgr::asset_source::memory} {
		create_from_assimp(meshes, create_collider_mesh);
    }

	mesh::mesh(const eastl::span<const vertex> vertices, const eastl::span<const index> indices, const bool create_collider_mesh) : asset{assetmgr::asset_source::memory} {
		create_from_data(vertices, indices, create_collider_mesh);
	}

    auto mesh::create_from_assimp(const eastl::span<const aiMesh*> meshes, const bool create_collider_mesh) -> void {
		std::size_t num_vertices = 0, num_indices = 0;
		for (const aiMesh* mesh : meshes) {
			num_vertices += mesh->mNumVertices;
			num_indices += mesh->mNumFaces * 3;
		}
		eastl::vector<vertex> vertices {};
		eastl::vector<index> indices {};
		vertices.reserve(num_vertices);
		indices.reserve(num_indices);
		m_primitives.reserve(meshes.size());
		for (const aiMesh* mesh : meshes) {
			primitive prim_info {};
			load_primitive(vertices, indices, mesh, prim_info, m_aabb);
			m_primitives.emplace_back(prim_info);
		}
		m_primitives.shrink_to_fit();
		compute_aabb(m_aabb, vertices);
		create_buffers(vertices, indices);
		m_approx_byte_size = sizeof(*this)
			+ m_vertex_buffer->get_size() * sizeof(vertices[0])
			+ m_index_buffer->get_size() * sizeof(indices[0])
			+ m_primitives.size() * sizeof(m_primitives[0]);
        if (create_collider_mesh) {
            m_collision_mesh.emplace(physics::collider::new_mesh(vertices, indices));
        }
    }

	auto mesh::create_from_data(eastl::span<const vertex> vertices, eastl::span<const index> indices, const bool create_collider_mesh) -> void {
		m_primitives.shrink_to_fit();
		m_primitives.emplace_back(primitive {
			.index_start = 0,
			.index_count = static_cast<std::uint32_t>(indices.size()),
			.vertex_start = 0,
			.vertex_count = static_cast<std::uint32_t>(vertices.size()),
		});
		compute_aabb(m_aabb, vertices);
		m_primitives[0].aabb = m_aabb;
		create_buffers(vertices, indices);
		m_approx_byte_size = sizeof(*this)
			+ m_vertex_buffer->get_size() * sizeof(vertices[0])
			+ m_index_buffer->get_size() * sizeof(indices[0])
			+ m_primitives.size() * sizeof(m_primitives[0]);
		if (create_collider_mesh) {
			m_collision_mesh.emplace(physics::collider::new_mesh(vertices, indices));
		}
	}

    auto mesh::create_buffers(const eastl::span<const vertex> vertices, const eastl::span<const index> indices) -> void {
    	panic_assert(indices.size() <= eastl::numeric_limits<index>::max());

    	m_vertex_buffer.emplace(
			vertices.size() * sizeof(vertices[0]),
			0,
			vk::BufferUsageFlagBits::eVertexBuffer,
			VMA_MEMORY_USAGE_GPU_ONLY,
			0,
			vertices.data()
		);
        m_vertex_count = static_cast<std::uint32_t>(vertices.size());

    	if (indices.size() <= eastl::numeric_limits<std::uint16_t>::max()) { // 16 bit indices
    		eastl::vector<std::uint16_t> indices16 {};
    		indices16.reserve(indices.size());
    		for (const index idx : indices) {
    			indices16.emplace_back(static_cast<std::uint16_t>(idx));
    		}
    		m_index_32bit = false;
    		m_index_count = static_cast<std::uint32_t>(indices16.size());
    		m_index_buffer.emplace(
				indices16.size() * sizeof(indices16[0]),
				0,
				vk::BufferUsageFlagBits::eIndexBuffer,
				VMA_MEMORY_USAGE_GPU_ONLY,
				0,
				indices16.data()
			);
    	} else { // 32 bit indices
    		m_index_32bit = true;
    		m_index_count = static_cast<std::uint32_t>(indices.size());
    		m_index_buffer.emplace(
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
