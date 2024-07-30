// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"

LUA_INTEROP_API auto __lu_entity_is_valid(const lua_entity_id id) -> bool {
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    return ent.has_value();
}

LUA_INTEROP_API auto __lu_entity_get_name(const lua_entity_id id) -> const char* {
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return "Null Entity"; }
    return ent->name().c_str();
}

LUA_INTEROP_API auto __lu_entity_set_name(const lua_entity_id id, const char* name) -> void {
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return; }
    ent->set_name(name);
}

LUA_INTEROP_API auto __lu_entity_get_flags(const lua_entity_id id) -> std::uint32_t {
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return 0; }
    return ent->get<com::metadata>()->flags;
}

LUA_INTEROP_API auto __lu_entity_set_flags(const lua_entity_id id, const std::uint32_t flags) -> void {
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return; }
    ent->get_mut<com::metadata>()->flags = flags;
}

