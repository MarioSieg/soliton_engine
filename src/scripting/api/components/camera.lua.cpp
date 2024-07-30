// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "../_prelude.hpp"

impl_component_core(camera)

LUA_INTEROP_API auto __lu_com_camera_get_fov(const lua_entity_id id) -> double {
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return false; }
    if (const auto* camera = ent->get_mut<const com::camera>(); camera) [[likely]] {
        return camera->fov;
    }
    return 0.0;
}

LUA_INTEROP_API auto __lu_com_camera_set_fov(const lua_entity_id id, const double fov) -> void {
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return; }
    if (auto* camera = ent->get_mut<com::camera>(); camera) [[likely]] {
        camera->fov = static_cast<float>(fov);
    }
}

LUA_INTEROP_API auto __lu_com_camera_get_near_clip(const lua_entity_id id) -> double {
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return 0.0; }
    if (const auto* camera = ent->get_mut<const com::camera>(); camera) [[likely]] {
        return camera->z_clip_near;
    }
    return 0.0;
}

LUA_INTEROP_API auto __lu_com_camera_set_near_clip(const lua_entity_id id, const double near_clip) -> void {
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return; }
    if (auto* camera = ent->get_mut<com::camera>(); camera) [[likely]] {
        camera->z_clip_near = static_cast<float>(near_clip);
    }
}

LUA_INTEROP_API auto __lu_com_camera_get_far_clip(const lua_entity_id id) -> double {
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return 0.0; }
    if (const auto* camera = ent->get_mut<const com::camera>(); camera) [[likely]] {
        return camera->z_clip_far;
    }
    return 0.0;
}

LUA_INTEROP_API auto __lu_com_camera_set_far_clip(const lua_entity_id id, const double far_clip) -> void {
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return; }
    if (auto* camera = ent->get_mut<com::camera>(); camera) [[likely]] {
        camera->z_clip_far = static_cast<float>(far_clip);
    }
}
