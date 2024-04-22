// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "scene.hpp"
#include "../core/kernel.hpp"
#include "../graphics/mesh.hpp"
#include "../graphics/vulkancore/context.hpp"

#include <filesystem>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/StringComparison.h>
#include <assimp/IOSystem.hpp>
#include <assimp/DefaultIOStream.h>
#include <assimp/DefaultIOSystem.h>
#include <ankerl/unordered_dense.h>
#include <DirectXMath.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>
#include <bimg/bimg.h>
#include <bimg/decode.h>

namespace assetmgr {
    extern constinit std::atomic_size_t s_asset_requests;
    extern constinit std::atomic_size_t s_total_bytes_loaded;
};

class lunam_io_stream : public Assimp::DefaultIOStream {
public:
    lunam_io_stream(std::FILE* pFile, const std::string& strFilename) : Assimp::DefaultIOStream{pFile, strFilename} {}
    auto Read(void* pvBuffer, size_t pSize, size_t pCount) -> size_t override {
        assetmgr::s_total_bytes_loaded.fetch_add(pSize * pCount, std::memory_order_relaxed);
        return Assimp::DefaultIOStream::Read(pvBuffer, pSize, pCount);
    }
};

class lunam_assimp_io_system : public Assimp::DefaultIOSystem {
public:
    auto Open(const char* file, const char* mode = "rb") -> Assimp::IOStream* override {
        assetmgr::s_asset_requests.fetch_add(1, std::memory_order_relaxed);
        std::FILE* f = std::fopen(file, mode);
        if (!f) [[unlikely]] {
            log_error("Failed to open file '{}'", file);
            return nullptr;
        }
        return new lunam_io_stream {f, file};
    }
    auto Close(Assimp::IOStream* f) -> void override {
        delete f;
    }
};

auto scene::import_from_file(const std::string& path, const float scale, const std::uint32_t load_flags) -> void {
    class assimp_logger final : public Assimp::LogStream {
        auto write(const char* message) -> void override {
            const auto len = std::strlen(message);
            auto* copy = static_cast<char*>(alloca(len));
            std::memcpy(copy, message, len);
            copy[len-1] = '\0'; // replace \n with \0
            log_info("[WorldImporter]: {}", copy);
        }
    };

    log_info("Importing scene from file '{}'", path);

    Assimp::DefaultLogger::create("", Assimp::Logger::NORMAL);
    Assimp::DefaultLogger::get()->attachStream(new assimp_logger {}, Assimp::Logger::Info | Assimp::Logger::Err | Assimp::Logger::Warn);

    const auto start = std::chrono::high_resolution_clock::now();

    Assimp::Importer importer {};
    importer.SetIOHandler(new lunam_assimp_io_system{}); // use my IO system
    passert(importer.ValidateFlags(load_flags));
    const aiScene* scene = importer.ReadFile(path.c_str(), load_flags);
    if (!scene || !scene->mNumMeshes) [[unlikely]] {
        panic("Failed to load scene from file '{}': {}", path, importer.GetErrorString());
    }

    const std::string asset_root = std::filesystem::path {path}.parent_path().string() + "/";

    auto* missing_material = get_asset_registry<graphics::material>().load_from_memory();
    missing_material->albedo_map = get_asset_registry<graphics::texture>().load("assets/textures/system/error.png");
    missing_material->flush_property_updates();

    ankerl::unordered_dense::map<std::string, std::uint32_t> resolved_names {};

    std::uint32_t num_nodes = 0;
    std::function<auto (aiNode*) -> void> visitor = [&](aiNode* node) -> void {
        if (!node) [[unlikely]] {
            return;
        }
        if (node->mParent) {
            node->mTransformation = node->mParent->mTransformation * node->mTransformation;
        }

        ++num_nodes;

        for (unsigned i = 0; i < node->mNumMeshes; ++i) {
            std::string name {node->mName.C_Str()};
            if (resolved_names.contains(name)) {
                name += '_';
                name += std::to_string(++resolved_names[name]);
            } else {
                resolved_names[name] = 0;
            }
            const flecs::entity e = spawn(name.c_str());
            auto* metadata = e.get_mut<com::metadata>();
            metadata->flags |= com::entity_flags::static_object; // make everything static by default

            auto* transform = e.get_mut<com::transform>();
            aiVector3D scaling, position;
            aiQuaternion rotation;
            node->mTransformation.Decompose(scaling, rotation, position);
            transform->position = {position.x, position.y, position.z, 0.0f};
            transform->rotation = {rotation.x, rotation.y, rotation.z, rotation.w};
            transform->scale = {scaling.x, scaling.y, scaling.z, 0.0f};

            auto* renderer = e.get_mut<com::mesh_renderer>();

            const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
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

            std::span span {&mesh, 1};
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
