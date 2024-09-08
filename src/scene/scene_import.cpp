// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "scene.hpp"
#include "../core/kernel.hpp"
#include "../graphics/mesh.hpp"
#include "../graphics/material.hpp"
#include "../graphics/utils/assimp_utils.hpp"
#include "../graphics/vulkancore/context.hpp"

#include <filesystem>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <ankerl/unordered_dense.h>
#include <DirectXMath.h>
#include <bimg/bimg.h>
#include <bimg/decode.h>

namespace lu {
    auto scene::import_from_file(const eastl::string& path, const float scale, const std::uint32_t load_flags) -> void {
        log_info("Importing scene from file '{}'", path);

        Assimp::DefaultLogger::create("", Assimp::Logger::NORMAL);
        Assimp::DefaultLogger::get()->attachStream(new graphics::assimp_logger {}, Assimp::Logger::Info | Assimp::Logger::Err | Assimp::Logger::Warn);

        const auto start = eastl::chrono::high_resolution_clock::now();

        Assimp::Importer importer {};
        //importer.SetIOHandler(new graphics::lunam_assimp_io_system {});
        passert(importer.ValidateFlags(load_flags));
        const aiScene* scene = importer.ReadFile(path.c_str(), load_flags);
        if (!scene || !scene->mNumMeshes) [[unlikely]] {
            panic("Failed to load scene from file '{}': {}", path, importer.GetErrorString());
        }

        eastl::string asset_root = std::filesystem::path{path.c_str()}.parent_path().string().c_str();
        str_replace(asset_root, "engine_assets", "RES");
        asset_root += "/";

        ankerl::unordered_dense::map<eastl::string, std::uint32_t> resolved_names {};

        std::uint32_t num_nodes = 0;
        eastl::function<auto (aiNode*) -> void> visitor = [&](aiNode* node) -> void {
            if (!node) [[unlikely]] {
                return;
            }
            if (node->mParent) {
                node->mTransformation = node->mParent->mTransformation * node->mTransformation;
            }

            ++num_nodes;

            for (unsigned i = 0; i < node->mNumMeshes; ++i) {
                eastl::string name {node->mName.C_Str()};
                if (resolved_names.contains(name)) {
                    name += '_';
                    name += eastl::to_string(++resolved_names[name]);
                } else {
                    resolved_names[name] = 0;
                }
                const flecs::entity e = spawn(name.c_str());
                auto* metadata = e.get_mut<com::metadata>();

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

                auto load_tex = [&, binding = 0u](const std::initializer_list<aiTextureType> types) mutable -> graphics::material_property {
                    for (auto textureType = types.begin(); textureType != types.end(); std::advance(textureType, 1)) {
                        if (!mat->GetTextureCount(*textureType)) [[unlikely]] continue;
                        aiString name {};
                        if (mat->Get(AI_MATKEY_TEXTURE(*textureType, 0), name) != AI_SUCCESS) {
                            continue;
                        }
                        std::filesystem::path tex_path = "/";
                        tex_path += std::filesystem::relative((asset_root + name.C_Str()).c_str()).string().c_str();
                        if (!tex_path.has_extension()) [[unlikely]] {
                            continue;
                        }
                        eastl::string path = tex_path.c_str();
                        // replace backslashes with forward slashes
                        eastl::replace(path.begin(), path.end(), '\\', '/');
                        return graphics::material_property {
                            binding++,
                            get_asset_registry<graphics::texture>().load(eastl::move(path))
                        };
                    }
                    log_warn("No texture found for material");
                    return graphics::material_property {
                        binding++,
                        get_asset_registry<graphics::texture>().load(graphics::sv_error_texture())
                    };
                };

                auto& registry = get_asset_registry<graphics::material>();
                auto [material_ref, material] = get_asset_registry<graphics::material>().insert();
                material->properties["tex_albedo"]      = load_tex({aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR});
                material->properties["tex_normal"]      = load_tex({aiTextureType_NORMALS, aiTextureType_NORMAL_CAMERA});
                material->properties["tex_roughness"]   = load_tex({aiTextureType_SPECULAR, aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_SHININESS});
                material->properties["tex_height"]      = load_tex({aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR});
                material->properties["tex_ao"]          = load_tex({aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR});
                material->properties["tex_emission"]    = load_tex({aiTextureType_EMISSION_COLOR, aiTextureType_EMISSIVE});
                material->flush_property_updates();
                material->print_properties();

                renderer->materials.emplace_back(material);

                eastl::span span {&mesh, 1};
                auto& mesh_registry = get_asset_registry<graphics::mesh>();
                renderer->meshes.emplace_back(mesh_registry.insert(eastl::make_unique<graphics::mesh>(span)).second);
            }
            for (unsigned i = 0; i < node->mNumChildren; ++i) {
                visitor(node->mChildren[i]);
            }
        };

        visitor(scene->mRootNode);

        Assimp::DefaultLogger::kill();

        log_info("Imported scene from file '{}', {} nodes in {:.03}s", path, num_nodes, eastl::chrono::duration_cast<eastl::chrono::duration<double>>(eastl::chrono::high_resolution_clock::now() - start).count());
    }
}
