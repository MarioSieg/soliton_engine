// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "scene.hpp"
#include "../core/kernel.hpp"
#include "../graphics/mesh.hpp"
#include "../graphics/vulkancore/context.hpp"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <ankerl/unordered_dense.h>
#include <DirectXMath.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>
#include <bimg/bimg.h>
#include <bimg/decode.h>

struct proxy final : scene {
    template <typename... Ts>
    explicit proxy(Ts&&... args) : scene(std::forward<Ts>(args)...) {}
};

static constinit std::atomic_uint32_t id_gen = 1;

scene::scene() : id{id_gen.fetch_add(1, std::memory_order_relaxed)} {
    static const auto main_tid = std::this_thread::get_id();
    passert(main_tid == std::this_thread::get_id());
}

scene::~scene() {
    m_meshes.invalidate();
    m_textures.invalidate();
	vkcheck(vkb_vk_device().waitIdle());
	log_info("Destroyed scene '{}', id: {}", name, id);
}

auto scene::new_active(std::string&& name, std::string&& file, const float scale) -> void {
    auto scene = std::make_unique<proxy>();
    scene->name = std::move(name);
    log_info("Created scene '{}', id: {}", scene->name, scene->id);
	if (!file.empty()) {
		scene->import_from_file(file, scale);
	}
    m_active = std::move(scene);
}

auto scene::on_tick() -> void {
    progress(static_cast<float>(kernel::get().get_delta_time()));
}

auto scene::on_start() -> void {
    kernel::get().on_new_scene_start(*this);
}

auto scene::spawn(const char* name) const -> flecs::entity {
    flecs::entity ent = this->entity(name);
    ent.add<c_metadata>();
    return ent;
}

auto scene::import_from_file(const std::string& path, const float scale) -> void {
    class assimp_logger final : public Assimp::LogStream {
        auto write(const char* message) -> void override {
            const auto len = std::strlen(message);
            auto* copy = static_cast<char*>(alloca(len));
            std::memcpy(copy, message, len);
            copy[len - 1] = '\0'; // replace \n with \0
            log_info("[WorldImporter]: {}", copy);
        }
    };

    log_info("Importing scene from file '{}'", path);

    Assimp::DefaultLogger::create("", Assimp::Logger::NORMAL);
    Assimp::DefaultLogger::get()->attachStream(new assimp_logger {}, Assimp::Logger::Info | Assimp::Logger::Err | Assimp::Logger::Warn);

    const auto start = std::chrono::high_resolution_clock::now();

    Assimp::Importer importer {};
    const aiScene* scene = importer.ReadFile(path.c_str(), graphics::mesh::k_import_flags);
    if (!scene || !scene->mNumMeshes) [[unlikely]] {
        panic("Failed to load scene from file '{}': {}", path, importer.GetErrorString());
    }

    const std::string asset_root = std::filesystem::path {path}.parent_path().string() + "/";

    auto* missing_material = get_asset_registry<graphics::material>().load_from_memory();
    missing_material->albedo_map = get_asset_registry<graphics::texture>().load("assets/textures/system/error.png");
    missing_material->flush_property_updates();

    std::uint32_t num_nodes = 0;
    std::function<auto (aiNode*) -> void> visitor = [&](aiNode* node) -> void {
        if (!node) [[unlikely]] {
            return;
        }
        if (node->mParent) {
            node->mTransformation = node->mParent->mTransformation * node->mTransformation;
        }

        ++num_nodes;

        const flecs::entity e = spawn(nullptr);
        auto* metadata = e.get_mut<c_metadata>();
        metadata->name = node->mName.C_Str();

        auto* transform = e.get_mut<c_transform>();
        aiVector3D scaling, position;
        aiQuaternion rotation;
        node->mTransformation.Decompose(scaling, rotation, position);
        transform->position = {position.x, position.y, position.z};
        transform->rotation = {rotation.x, rotation.y, rotation.z, rotation.w};
        transform->scale = {scaling.x, scaling.y, scaling.z};

        if (node->mNumMeshes > 0) {
            auto* renderer = e.get_mut<c_mesh_renderer>();

            std::vector<const aiMesh*> meshes {};
            meshes.reserve(node->mNumMeshes);
            for (unsigned i = 0; i < node->mNumMeshes; ++i) {
                const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                meshes.emplace_back(mesh);
                const aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

                const auto load_tex = [&](const std::initializer_list<aiTextureType> types) -> graphics::texture* {
                    for (auto textureType = types.begin(); textureType != types.end(); std::advance(textureType, 1)) {
                        if (!mat->GetTextureCount(*textureType)) [[unlikely]] { continue; }
                        aiString name {};
                        mat->Get(AI_MATKEY_TEXTURE(*textureType, 0), name);
                        std::string tex_path = asset_root + name.C_Str();
                        return get_asset_registry<graphics::texture>().load(std::move(tex_path));
                    }
                    return nullptr;
                };

                auto* material = get_asset_registry<graphics::material>().load_from_memory();
                material->albedo_map = load_tex({aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR});
                material->normal_map = load_tex({aiTextureType_NORMALS, aiTextureType_NORMAL_CAMERA});
                material->flush_property_updates();

                renderer->materials.emplace_back(material);
            }

            std::span span {meshes};
            renderer->meshes.emplace_back(get_asset_registry<graphics::mesh>().load_from_memory(span));
        }
        for (unsigned i = 0; i < node->mNumChildren; ++i) {
            visitor(node->mChildren[i]);
        }
    };

    visitor(scene->mRootNode);

    Assimp::DefaultLogger::kill();

    log_info("Imported scene from file '{}', {} nodes in {:.03}s", path, num_nodes, std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start).count());
}
