// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../_prelude.hpp"

impl_component_core(transform)

LUA_INTEROP_API auto __lu_com_transform_set_pos(const flecs::id_t id, const double x, const double y, const double z) -> void {
    const flecs::entity ent {scene::get_active(), id};
    if (auto* transform = ent.get_mut<com::transform>(); transform) [[likely]] {
        transform->position = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z)
        };
    }
}

LUA_INTEROP_API auto __lu_com_transform_get_pos(const flecs::id_t id) -> lua_vec3 {
    const flecs::entity ent {scene::get_active(), id};
    if (const auto* transform = ent.get_mut<const com::transform>(); transform) [[likely]] {
        return transform->position;
    }
    return {};
}

LUA_INTEROP_API auto __lu_com_transform_set_rot(const flecs::id_t id, const double x, const double y, const double z, const double w) -> void {
    const flecs::entity ent {scene::get_active(), id};
    if (auto* transform = ent.get_mut<com::transform>(); transform) [[likely]] {
        transform->rotation = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z),
            static_cast<float>(w)
        };
    }
}

LUA_INTEROP_API auto __lu_com_transform_get_rot(const flecs::id_t id) -> lua_vec4 {
    const flecs::entity ent {scene::get_active(), id};
    if (const auto* transform = ent.get_mut<const com::transform>(); transform) [[likely]] {
        return transform->rotation;
    }
    return {};
}

LUA_INTEROP_API auto __lu_com_transform_set_scale(const flecs::id_t id, const double x, const double y, const double z) -> void {
    const flecs::entity ent {scene::get_active(), id};
    if (auto* transform = ent.get_mut<com::transform>(); transform) [[likely]] {
        transform->scale = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z)
        };
    }
}

LUA_INTEROP_API auto __lu_com_transform_get_scale(const flecs::id_t id) -> lua_vec3 {
    const flecs::entity ent {scene::get_active(), id};
    if (const auto* transform = ent.get_mut<const com::transform>(); transform) [[likely]] {
        return transform->scale;
    }
    return {};
}
