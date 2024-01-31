// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "scene.hpp"
#include "../core/kernel.hpp"
#include "../graphics/mesh.hpp"
#include "../graphics/vulkancore/context.hpp"
#include <tiny_gltf.h>
#include <ankerl/unordered_dense.h>
#include <DirectXMath.h>

struct proxy final : scene {
    template <typename... Ts>
    explicit proxy(Ts&&... args) : scene(std::forward<Ts>(args)...) {}
};

static constinit std::atomic_uint32_t id_gen = 1;

scene::scene() : id{id_gen.fetch_add(1, std::memory_order_relaxed)} {
    static const auto main_tid = std::this_thread::get_id();
    passert(main_tid == std::this_thread::get_id());
    m_eitbl.reserve(0x1000);

	m_meshes.invalidate();
	m_textures.invalidate();
}

scene::~scene() {
	vkcheck(vkb_vk_device().waitIdle());
	m_eitbl.clear();
	log_info("Destroyed scene '{}', id: {}", name, id);
}

auto scene::new_active(std::string&& name, std::string&& gltf_file, const float scale) -> void {
    auto scene = std::make_unique<proxy>();
    scene->name = std::move(name);
    log_info("Created scene '{}', id: {}", scene->name, scene->id);
	if (!gltf_file.empty()) {
		scene->load_from_gltf(gltf_file, scale);
	}
    m_active = std::move(scene);
}

auto scene::on_tick() -> void {
    progress(static_cast<float>(kernel::get().get_delta_time()));
}

auto scene::on_start() -> void {
    kernel::get().on_new_scene_start(*this);
}

auto scene::spawn(const char* name, lua_entity* l_id) -> flecs::entity {
    flecs::entity ent = this->entity(name);
    ent.add<c_metadata>();
    ent.add<c_transform>();
    if (l_id) {
    	*l_id = m_eitbl.size() & ~0u;
    }
    m_eitbl.emplace_back(ent);
    passert(m_eitbl.size() <= k_max_entities);
    return ent;
}

auto scene::load_from_gltf(const std::string& path, const float scale) -> void {
	tinygltf::Model model {};
	tinygltf::TinyGLTF loader {};
	tinygltf::FsCallbacks fs {};
	fs.FileExists = &tinygltf::FileExists;
	fs.ExpandFilePath = &tinygltf::ExpandFilePath;
	fs.ReadWholeFile = +[](std::vector<unsigned char>* out, [[maybe_unused]] std::string* err, const std::string& path, [[maybe_unused]] void* usr) -> bool {
		return assetmgr::load_asset_blob_raw(path, *out);
	};
	fs.WriteWholeFile = &tinygltf::WriteWholeFile;
	fs.GetFileSizeInBytes = &tinygltf::GetFileSizeInBytes;
	loader.SetFsCallbacks(fs);

	std::string err {};
	std::string warn {};
	const bool is_bin = std::filesystem::path{path}.extension() == ".glb";
	bool result;
	if (is_bin) {
		result = loader.LoadBinaryFromFile(&model, &err, &warn, path);
	} else {
		result = loader.LoadASCIIFromFile(&model, &err, &warn, path);
	}
	if (!warn.empty()) [[unlikely]] {
		printf("Warn: %s\n", warn.c_str());
		log_warn("GLTF import: {}", warn);
	}
	if (!err.empty()) [[unlikely]] {
		log_error("GLTF import: {}", err);
	}
	passert(result);
	passert(!model.scenes.empty());

	const DirectX::XMMATRIX scale_mtx = DirectX::XMMatrixScalingFromVector(DirectX::XMVectorReplicate(scale));
	std::uint32_t num_nodes, num_meshes = 0;
	std::function<auto (const tinygltf::Node&) -> void> visitor = [&](const tinygltf::Node& node) {
		if (node.mesh > -1) [[likely]] {
			++num_nodes;
			const flecs::entity ent = this->spawn(nullptr);
			const std::string name = node.name.empty() ? "unnamed" : node.name;
			auto* metadata = ent.get_mut<c_metadata>();
			metadata->name = name;

			DirectX::XMMATRIX local = DirectX::XMMatrixIdentity();
			if (node.matrix.size() == 16) {
				auto* dst = reinterpret_cast<float*>(&local);
				for (int i = 0; i < 16; ++i) {
					dst[i] = static_cast<float>(node.matrix[i]);
				}
			} else {
				if (node.translation.size() == 3) {
					const DirectX::XMMATRIX mm = DirectX::XMMatrixTranslation(
						static_cast<float>(node.translation[0]),
						static_cast<float>(node.translation[1]),
						static_cast<float>(node.translation[2])
					);
					local = XMMatrixMultiply(mm, local);
				}
				if (node.rotation.size() == 4) {
					const DirectX::XMMATRIX mm = DirectX::XMMatrixRotationQuaternion(DirectX::XMVectorSet(
						static_cast<float>(node.rotation[0]),
						static_cast<float>(node.rotation[1]),
						static_cast<float>(node.rotation[2]),
						static_cast<float>(node.rotation[3])
					));
					local = XMMatrixMultiply(mm, local);
				}
				if (node.scale.size() == 3) {
					const DirectX::XMMATRIX mm = DirectX::XMMatrixScaling(
						static_cast<float>(node.scale[0]),
						static_cast<float>(node.scale[1]),
						static_cast<float>(node.scale[2])
					);
					local = XMMatrixMultiply(mm, local);
				}
			}
			local = XMMatrixMultiply(local, scale_mtx);
			local = XMMatrixMultiply(local, DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(90.0f)));

			DirectX::XMVECTOR scale, rotation, translation;
			XMMatrixDecompose(&scale, &rotation, &translation, local);
			auto* transform = ent.get_mut<c_transform>();
			XMStoreFloat4(&transform->position, translation);
			XMStoreFloat4(&transform->rotation, rotation);
			XMStoreFloat4(&transform->scale, scale);

			const int idx = node.mesh;
			graphics::mesh* mesh = m_meshes.load(idx, model, model.meshes[idx]);
			auto* rendrer = ent.get_mut<c_mesh_renderer>();
			rendrer->mesh = mesh;
			++num_meshes;
		}
		for (const int child : node.children) {
			visitor(model.nodes[child]);
		}
	};

	const int scene_idx = model.defaultScene > -1 ? model.defaultScene : 0;
	const auto& scene = model.scenes[scene_idx];
	for (const int node : scene.nodes) {
		visitor(model.nodes[node]);
	}

	log_info("Loaded GLTF scene '{}', {} nodes, {} meshes", path, num_nodes, num_meshes);
}
