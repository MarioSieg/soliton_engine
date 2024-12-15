// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "../_prelude.hpp"

using graphics::mesh;

LUA_INTEROP_API auto __lu_mesh_load(const char* const resource_path, const std::uint32_t import_flags) -> assetmgr::interop_asset_id {
    return scene_mgr::active().get_asset_registry<mesh>().interop.load(resource_path, import_flags);
}

LUA_INTEROP_API auto __lu_mesh_get_primitive_count(const assetmgr::interop_asset_id id) -> std::uint32_t {
    get_resource_property(mesh, get_primitives().size(), 0);
}

LUA_INTEROP_API auto __lu_mesh_get_vertex_count(const assetmgr::interop_asset_id id) -> std::uint32_t {
    get_resource_property(mesh, get_vertex_count(), 0);
}

LUA_INTEROP_API auto __lu_mesh_get_index_count(const assetmgr::interop_asset_id id) -> std::uint32_t {
    get_resource_property(mesh, get_index_count(), 0);
}

LUA_INTEROP_API auto __lu_mesh_get_min_bound(const assetmgr::interop_asset_id id) -> lua_vec3 {
    const mesh* resource = scene_mgr::active().get_asset_registry<mesh>().interop[id];
    if (!resource) return {};
    XMVECTOR center = XMLoadFloat3(&resource->get_aabb().Center);
    XMVECTOR extents = XMLoadFloat3(&resource->get_aabb().Extents);
    XMFLOAT3A r;
    XMStoreFloat3A(&r, XMVectorSubtract(center, extents));
    return r;
}

LUA_INTEROP_API auto __lu_mesh_get_max_bound(const assetmgr::interop_asset_id id) -> lua_vec3 {
    const mesh* resource = scene_mgr::active().get_asset_registry<mesh>().interop[id];
    if (!resource) return {};
    XMVECTOR center = XMLoadFloat3(&resource->get_aabb().Center);
    XMVECTOR extents = XMLoadFloat3(&resource->get_aabb().Extents);
    XMFLOAT3A r;
    XMStoreFloat3A(&r, XMVectorAdd(center, extents));
    return r;
}

LUA_INTEROP_API auto __lu_mesh_has_32_bit_indices(assetmgr::interop_asset_id id) -> bool {
    get_resource_property(mesh, has_32bit_indices(), 0);
}

