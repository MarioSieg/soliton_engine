// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "mesh.hpp"
#include "../vulkancore/context.hpp"

#include <unordered_set>

// STB impls are in src/stb/stb_impls.cpp
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_ENABLE_DRACO
#define TINYGLTF_IMPLEMENTATION
#include "../gltf/tiny_gltf.h"

namespace graphics {
	[[nodiscard]] static auto load_primitive(
		std::vector<mesh::vertex>& vertices,
		std::vector<mesh::index>& indices,
		const tinygltf::Primitive& prim,
		const tinygltf::Model& model,
		mesh::primitive& prim_info
	) -> bool;

    mesh::mesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh) {
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
    }

    mesh::~mesh() {
    	m_index_buffer.destroy();
		m_vertex_buffer.destroy();
    }

    auto mesh::draw(const vk::CommandBuffer cmd) -> void {
    	constexpr vk::DeviceSize offsets = 0;
    	cmd.bindVertexBuffers(0, 1, &m_vertex_buffer.get_buffer(), &offsets);
    	cmd.bindIndexBuffer(m_index_buffer.get_buffer(), 0, m_index_32bit ? vk::IndexType::eUint32 : vk::IndexType::eUint16);
    	//for (const primitive& prim : m_primitives) {
    	//	cmd.drawIndexed(prim.index_count, 1, prim.first_index, 0, 1);
    	//}
    	cmd.drawIndexed(m_index_count, 1, 0, 0, 0);
    }

    auto mesh::recompute_bounds(const std::span<const vertex> vertices) -> void {
    	for (primitive& prim : m_primitives) {
    		XMVECTOR min = XMVectorReplicate(std::numeric_limits<float>::max());
    		XMVECTOR max = XMVectorReplicate(std::numeric_limits<float>::lowest());
    		for (std::size_t i = prim.vertex_start; i < prim.vertex_count; ++i) {
    			const vertex& v = vertices[i];
    			XMVECTOR pos = XMLoadFloat3(&v.position);
    			min = XMVectorMin(min, pos);
    			max = XMVectorMax(max, pos);
    		}
    		BoundingBox::CreateFromPoints(prim.aabb, min, max);
    		m_aabb.CreateMerged(m_aabb, m_aabb, prim.aabb);
    	}
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
            		.position = *reinterpret_cast<const XMFLOAT3*>(buffer_pos + i * 3),
            		.normal = buffer_normals ? *reinterpret_cast<const XMFLOAT3*>(buffer_normals + i * 3) : XMFLOAT3{0.0f, 0.0f, 0.0f},
            		.uv = buffer_tex_coords ? *reinterpret_cast<const XMFLOAT2*>(buffer_tex_coords + i * 2) : XMFLOAT2{0.0f, 0.0f},
            		.tangent = buffer_tangents ? *reinterpret_cast<const XMFLOAT3*>(buffer_tangents + i * 3) : XMFLOAT3{0.0f, 0.0f, 0.0f},
            		.bitangent = XMFLOAT3{0.0f, 0.0f, 0.0f} // TODO
            	};
            	vertices.emplace_back(vv);
            }
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

        prim_info.index_start = index_start;
        prim_info.index_count = num_indices;
        prim_info.vertex_start = vertex_start;
        prim_info.vertex_count = num_vertices;
    	return true;
	}
}