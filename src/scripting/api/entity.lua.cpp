// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"

LUA_INTEROP_API auto __lu_entity_is_valid(const lua_entity id) -> bool {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]]
        return false;
    const entity ent = active->lookup_entity_via_lua_id(id);
    return ent && ent.is_valid();
}

LUA_INTEROP_API auto __lu_entity_is_alive(const lua_entity id) -> bool {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]]
        return false;
    const entity ent = active->lookup_entity_via_lua_id(id);
    return ent && ent.is_alive();
}

LUA_INTEROP_API auto __lu_entity_set_pos(const lua_entity id, const double x, const double y, const double z) -> void {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]]
        return;
    const entity ent = active->lookup_entity_via_lua_id(id);
    if (!ent) [[unlikely]]
        return;
    if (auto* transform = ent.get_mut<c_transform>(); transform) [[likely]] {
        transform->position = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z)
        };
    }
}

LUA_INTEROP_API auto __lu_entity_get_pos(const lua_entity id) -> lua_vec3 {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]]
        return {};
    const entity ent = active->lookup_entity_via_lua_id(id);
    if (!ent) [[unlikely]]
        return {};
    if (const auto* transform = ent.get<const c_transform>(); transform) [[likely]] {
        return lua_vec3{transform->position};
    }
    return {};
}

LUA_INTEROP_API auto __lu_entity_set_rot(const lua_entity id, const double x, const double y, const double z, double w) -> void {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]]
        return;
    const entity ent = active->lookup_entity_via_lua_id(id);
    if (!ent) [[unlikely]]
        return;
    if (auto* transform = ent.get_mut<c_transform>(); transform) [[likely]] {
        transform->rotation = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z),
            static_cast<float>(w)
        };
    }
}

LUA_INTEROP_API auto __lu_entity_get_rot(const lua_entity id) -> lua_vec4 {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]]
        return {};
    const entity ent = active->lookup_entity_via_lua_id(id);
    if (!ent) [[unlikely]]
        return {};
    if (const auto* transform = ent.get<const c_transform>(); transform) [[likely]] {
        return lua_vec4{transform->rotation};
    }
    return {};
}

LUA_INTEROP_API auto __lu_entity_set_scale(const lua_entity id, const double x, const double y, const double z) -> void {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]]
        return;
    const entity ent = active->lookup_entity_via_lua_id(id);
    if (!ent) [[unlikely]]
        return;
    if (auto* transform = ent.get_mut<c_transform>(); transform) [[likely]] {
        transform->scale = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z)
        };
    }
}

LUA_INTEROP_API auto __lu_entity_get_scale(const lua_entity id) -> lua_vec3 {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]]
        return {};
    const entity ent = active->lookup_entity_via_lua_id(id);
    if (!ent) [[unlikely]]
        return {};
    if (const auto* transform = ent.get<const c_transform>(); transform) [[likely]]
        return lua_vec3{transform->scale};
    return {};
}
