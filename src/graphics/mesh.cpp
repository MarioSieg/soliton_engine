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

    mesh::mesh(const std::string& path) {
        tinygltf::Model model {};
        tinygltf::TinyGLTF loader {};
        std::string err {};
        std::string warn {};
        const bool is_bin = std::filesystem::path{path}.extension() == ".glb";
        bool result;
        if (is_bin) {
            result = loader.LoadBinaryFromFile(&model, &err, &warn, path.c_str());
        } else {
            result = loader.LoadASCIIFromFile(&model, &err, &warn, path.c_str());
        }
        if (!warn.empty()) [[unlikely]] {
            printf("Warn: %s\n", warn.c_str());
            log_warn("GLTF import: {}", warn);
        }
        if (!err.empty()) [[unlikely]] {
            log_error("GLTF import: {}", err);
        }
		passert(result);

        std::vector<vertex> vertices {};
        std::vector<index> indices {};
    	std::unordered_set<int> already_loaded {};
    	already_loaded.reserve(model.meshes.size());

    	// count vertices to reserve memory
    	std::size_t acc_vertices = 0;
    	for (const tinygltf::Mesh& mesh : model.meshes) {
    		for (const tinygltf::Primitive& prim : mesh.primitives) {
    			if (prim.indices < 0) [[unlikely]] {
    				log_error("GLTF import: Mesh has no indices");
    				continue;
    			}
    			// Position attribute is required
    			if (!prim.attributes.contains("POSITION")) [[unlikely]] {
    				log_error("GLTF import: Mesh has no position attribute");
    				continue;
    			}

    			const tinygltf::Accessor& pos_accessor = model.accessors[prim.attributes.find("POSITION")->second];
    			acc_vertices += pos_accessor.count;
    		}
    	}

    	vertices.reserve(acc_vertices);
    	indices.reserve(acc_vertices);

    	std::uint32_t n = 0;
    	std::function<auto (const tinygltf::Node&) -> void> visitor = [&](const tinygltf::Node& node) {
    		for (const int child : node.children) {
    			visitor(model.nodes[child]);
    		}
    		log_info("Processing Node {}: {}", n++, node.name);
    		if (node.mesh < 0) [[unlikely]] {
    			return;
    		}
    		const int midx = node.mesh;
    		if (already_loaded.contains(midx)) {
    			return;
    		}

    		already_loaded.emplace(midx); // mark as loaded
    		const tinygltf::Mesh& mesh = model.meshes[midx];
    		XMVECTOR translation = XMVectorZero();
    		XMVECTOR rotation = XMQuaternionIdentity();
    		XMVECTOR scale = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
    		if (node.translation.size() == 3) {
    			translation = XMVectorSet(node.translation[0], node.translation[1], node.translation[2], 1.0f);
    		}
    		if (node.rotation.size() == 4) {
    			rotation = XMVectorSet(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
    		}
    		if (node.scale.size() == 3) {
    			scale = XMVectorSet(node.scale[0], node.scale[1], node.scale[2], 1.0f);
    		}
    		const XMMATRIX transform = XMMatrixTransformation(XMVectorZero(), XMVectorZero(), scale, XMVectorZero(), rotation, translation);
    		for (const tinygltf::Primitive& prim : mesh.primitives) {
    			primitive prim_info {};
    			if (load_primitive(vertices, indices, prim, model, prim_info)) {
    				m_primitives.emplace_back(prim_info);
    			}
    			// pre transform vertices:
    			for (std::span<vertex> verts {vertices.data() + prim_info.first_vertex, prim_info.vertex_count}; vertex& v : verts) {
    				XMStoreFloat3(&v.position, XMVector3Transform(XMLoadFloat3(&v.position), transform));
    			}
    		}
    	};

    	for (const tinygltf::Scene& scene : model.scenes) {
    		for (const int node : scene.nodes) {
    			visitor(model.nodes[node]);
    		}
    	}

    	passert(acc_vertices == vertices.size());
    	passert(indices.size() <= std::numeric_limits<index>::max());

    	m_vertex_buffer.create(
			vertices.size() * sizeof(vertices[0]),
			0,
			vk::BufferUsageFlagBits::eVertexBuffer,
			VMA_MEMORY_USAGE_GPU_ONLY,
			0,
			vertices.data()
		);

    	m_index_count = static_cast<std::uint32_t>(indices.size());
    	m_index_buffer.create(
			indices.size() * sizeof(indices[0]),
			0,
			vk::BufferUsageFlagBits::eIndexBuffer,
			VMA_MEMORY_USAGE_GPU_ONLY,
			0,
			indices.data()
		);

    	for (const primitive& prim : m_primitives) {
			BoundingBox::CreateMerged(aabb, aabb, prim.aabb);
		}
    }

    mesh::~mesh() {
    	m_index_buffer.destroy();
		m_vertex_buffer.destroy();
    }

    auto mesh::draw(vk::CommandBuffer cmd) -> void {
    	constexpr vk::DeviceSize offsets = 0;
    	cmd.bindVertexBuffers(0, 1, &m_vertex_buffer.get_buffer(), &offsets);
    	cmd.bindIndexBuffer(m_index_buffer.get_buffer(), 0, vk::IndexType::eUint32);
    	for (const primitive& prim : m_primitives) {
    		cmd.drawIndexed(prim.index_count, 1, prim.first_index, 0, 1);
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
        XMVECTOR pmin {}, pmax {};

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

			pmin = XMVectorSet(pos_accessor.minValues[0], pos_accessor.minValues[1], pos_accessor.minValues[2], 0.0f);
			pmax = XMVectorSet(pos_accessor.maxValues[0], pos_accessor.maxValues[1], pos_accessor.maxValues[2], 0.0f);

            const float* buffer_pos = nullptr;
            const float* buffer_normals = nullptr;
            const float* buffer_tex_coords = nullptr;
            const float* buffer_tangents = nullptr;
			buffer_pos = reinterpret_cast<const float *>(&(model.buffers[pos_view.buffer].data[pos_accessor.byteOffset + pos_view.byteOffset]));
			//posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
			//posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);

			if (prim.attributes.contains("NORMAL")) {
				const tinygltf::Accessor& norm_accessor = model.accessors[prim.attributes.find("NORMAL")->second];
				const tinygltf::BufferView& norm_view = model.bufferViews[norm_accessor.bufferView];
				buffer_normals = reinterpret_cast<const float *>(&(model.buffers[norm_view.buffer].data[norm_accessor.byteOffset + norm_view.byteOffset]));
			}
			if (prim.attributes.contains("TEXCOORD_0")) {
				const tinygltf::Accessor& uv_accessor = model.accessors[prim.attributes.find("TEXCOORD_0")->second];
				const tinygltf::BufferView& uv_view = model.bufferViews[uv_accessor.bufferView];
				buffer_tex_coords = reinterpret_cast<const float *>(&(model.buffers[uv_view.buffer].data[uv_accessor.byteOffset + uv_view.byteOffset]));
			}
			if (prim.attributes.contains("TANGENT")) {
				const tinygltf::Accessor& tangent_accessor = model.accessors[prim.attributes.find("TANGENT")->second];
				const tinygltf::BufferView& tangent_view = model.bufferViews[tangent_accessor.bufferView];
				buffer_tangents = reinterpret_cast<const float *>(&(model.buffers[tangent_view.buffer].data[tangent_accessor.byteOffset + tangent_view.byteOffset]));
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
            		auto* buf = new std::uint32_t[accessor.count];
            		std::memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
            		for (std::size_t i = 0; i < accessor.count; ++i) {
            			indices.emplace_back(buf[i] + vertex_start);
            		}
            		delete[] buf;
            	} break;
            	case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
		            auto* buf = new std::uint16_t[accessor.count];
            		std::memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
            		for (std::size_t i = 0; i < accessor.count; ++i) {
            			indices.emplace_back(buf[i] + vertex_start);
            		}
            		delete[] buf;
            	} break;
            	case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
            		auto* buf = new std::uint8_t[accessor.count];
            		std::memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
            		for (std::size_t i = 0; i < accessor.count; ++i) {
            			indices.emplace_back(buf[i] + vertex_start);
            		}
            		delete[] buf;
            	} break;
            	default:
            		log_error("GLTF import: Index component type {} not supported", accessor.componentType);
            }
        }

        prim_info.first_index = index_start;
        prim_info.index_count = num_indices;
        prim_info.first_vertex = vertex_start;
        prim_info.vertex_count = num_vertices;
        BoundingBox::CreateFromPoints(prim_info.aabb, pmin, pmax);
    	return true;
	}
}
