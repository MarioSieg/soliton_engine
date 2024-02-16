// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../_prelude.hpp"

impl_component_core(camera)

LUA_INTEROP_API auto __lu_com_camera_get_fov(const flecs::id_t id) -> double {
    const flecs::entity ent {scene::get_active(), id};
    if (const auto* camera = ent.get_mut<const com::camera>(); camera) [[likely]] {
        return camera->fov;
    }
    return 0.0;
}

LUA_INTEROP_API auto __lu_com_camera_set_fov(const flecs::id_t id, const double fov) -> void {
    const flecs::entity ent {scene::get_active(), id};
    if (auto* camera = ent.get_mut<com::camera>(); camera) [[likely]] {
        camera->fov = static_cast<float>(fov);
    }
}

LUA_INTEROP_API auto __lu_com_camera_get_near_clip(const flecs::id_t id) -> double {
    const flecs::entity ent {scene::get_active(), id};
    if (const auto* camera = ent.get_mut<const com::camera>(); camera) [[likely]] {
        return camera->z_clip_near;
    }
    return 0.0;
}

LUA_INTEROP_API auto __lu_com_camera_set_near_clip(const flecs::id_t id, const double near_clip) -> void {
    const flecs::entity ent {scene::get_active(), id};
    if (auto* camera = ent.get_mut<com::camera>(); camera) [[likely]] {
        camera->z_clip_near = static_cast<float>(near_clip);
    }
}

LUA_INTEROP_API auto __lu_com_camera_get_far_clip(const flecs::id_t id) -> double {
    const flecs::entity ent {scene::get_active(), id};
    if (const auto* camera = ent.get_mut<const com::camera>(); camera) [[likely]] {
        return camera->z_clip_far;
    }
    return 0.0;
}

LUA_INTEROP_API auto __lu_com_camera_set_far_clip(const flecs::id_t id, const double far_clip) -> void {
    const flecs::entity ent {scene::get_active(), id};
    if (auto* camera = ent.get_mut<com::camera>(); camera) [[likely]] {
        camera->z_clip_far = static_cast<float>(far_clip);
    }
}
