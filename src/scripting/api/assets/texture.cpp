// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "../_prelude.hpp"

using graphics::texture;

LUA_INTEROP_API auto __lu_texture_load(const char* const resource_path) -> assetmgr::interop_asset_id {
    return scene_mgr::active().get_asset_registry<texture>().interop.load(resource_path);
}

LUA_INTEROP_API auto __lu_texture_get_width(assetmgr::interop_asset_id id) -> std::uint32_t {
    get_resource_property(texture, width(), 0);
}

LUA_INTEROP_API auto __lu_texture_get_height(assetmgr::interop_asset_id id) -> std::uint32_t {
    get_resource_property(texture, height(), 0);
}

LUA_INTEROP_API auto __lu_texture_get_depth(assetmgr::interop_asset_id id) -> std::uint32_t {
    get_resource_property(texture, depth(), 0);
}

LUA_INTEROP_API auto __lu_texture_get_mip_levels(assetmgr::interop_asset_id id) -> std::uint32_t {
    get_resource_property(texture, mip_levels(), 0);
}

LUA_INTEROP_API auto __lu_texture_get_array_size(assetmgr::interop_asset_id id) -> std::uint32_t {
    get_resource_property(texture, array_size(), 0);
}

LUA_INTEROP_API auto __lu_texture_is_cubemap(assetmgr::interop_asset_id id) -> bool {
    get_resource_property(texture, is_cubemap(), false);
}

LUA_INTEROP_API auto __lu_texture_get_format(assetmgr::interop_asset_id id) -> std::uint32_t {
    get_resource_property(texture, format_int(), 0);
}
