// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"

LUA_INTEROP_API auto __lu_entity_is_valid(const flecs::id_t id) -> bool {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]] {
        return false;
    }
    const flecs::entity ent {*active, id};
    return ent && ent.is_valid();
}

LUA_INTEROP_API auto __lu_entity_is_alive(const flecs::id_t id) -> bool {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]] {
        return false;
    }
    const flecs::entity ent {*active, id};
    return ent && ent.is_alive();
}

LUA_INTEROP_API auto __lu_entity_set_pos(const flecs::id_t id, const double x, const double y, const double z) -> void {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]] {
        return;
    }
    const flecs::entity ent {*active, id};
    if (!ent) [[unlikely]] {
        return;
    }
    auto* transform = ent.get_mut<c_transform>();
    transform->position = {
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(z)
    };
}

LUA_INTEROP_API auto __lu_entity_get_pos(const flecs::id_t id) -> lua_vec3 {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]] {
        return {};
    }
    const flecs::entity ent {*active, id};
    if (!ent) [[unlikely]] {
        return {};
    }
    if (const auto* transform = ent.get<const c_transform>(); transform) [[likely]] {
        return lua_vec3{transform->position};
    }
    return {};
}

LUA_INTEROP_API auto __lu_entity_set_rot(const flecs::id_t id, const double x, const double y, const double z, double w) -> void {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]] {
        return;
    }
    const flecs::entity ent {*active, id};
    if (!ent) [[unlikely]] {
        return;
    }
    auto* transform = ent.get_mut<c_transform>();
    transform->rotation = {
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(z),
        static_cast<float>(w)
    };
}

LUA_INTEROP_API auto __lu_entity_get_rot(const flecs::id_t id) -> lua_vec4 {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]] {
        return {};
    }
    const flecs::entity ent {*active, id};
    if (!ent) [[unlikely]] {
        return {};
    }
    if (const auto* transform = ent.get<const c_transform>(); transform) [[likely]] {
        return lua_vec4{transform->rotation};
    }
    return {};
}

LUA_INTEROP_API auto __lu_entity_set_scale(const flecs::id_t id, const double x, const double y, const double z) -> void {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]] {
        return;
    }
    const flecs::entity ent {*active, id};
    if (!ent) [[unlikely]] {
        return;
    }
    auto* transform = ent.get_mut<c_transform>();
    transform->scale = {
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(z),
    };
}

LUA_INTEROP_API auto __lu_entity_get_scale(const flecs::id_t id) -> lua_vec3 {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]] {
        return {};
    }
    const flecs::entity ent {*active, id};
    if (!ent) [[unlikely]] {
        return {};
    }
    if (const auto* transform = ent.get<const c_transform>(); transform) [[likely]] {
        return lua_vec3{transform->scale};
    }
    return {};
}
