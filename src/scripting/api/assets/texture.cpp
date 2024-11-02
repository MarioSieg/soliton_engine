// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "../_prelude.hpp"

LUA_INTEROP_API auto __lu_texture_load(const char* const resource_path) -> lua_asset_id {
    auto& scene = scene_mgr::active();
    return scene.get_asset_registry<graphics::texture>().load(resource_path);
}
