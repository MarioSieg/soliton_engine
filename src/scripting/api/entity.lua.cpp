// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"

LUA_INTEROP_API auto __lu_entity_is_valid(const flecs::id_t id) -> bool {
    const flecs::entity ent {scene::get_active(), id};
    return ent && ent.is_valid();
}

LUA_INTEROP_API auto __lu_entity_is_alive(const flecs::id_t id) -> bool {
    const flecs::entity ent {scene::get_active(), id};
    return ent && ent.is_alive();
}

LUA_INTEROP_API auto __lu_entity_get_name(const flecs::id_t id) -> const char* {
    const flecs::entity ent {scene::get_active(), id};
    if (!ent.is_alive()) [[unlikely]] return {};
    return ent.name().c_str();
}

LUA_INTEROP_API auto __lu_entity_set_name(const flecs::id_t id, const char* name) -> void {
    flecs::entity ent {scene::get_active(), id};
    if (!ent.is_alive()) [[unlikely]] return;
    ent.set_name(name);
}

LUA_INTEROP_API auto __lu_entity_get_flags(const flecs::id_t id) -> std::uint32_t {
    const flecs::entity ent {scene::get_active(), id};
    if (!ent.is_alive()) [[unlikely]] return {};
    return ent.get<com::metadata>()->flags;
}

LUA_INTEROP_API auto __lu_entity_set_flags(const flecs::id_t id, const std::uint32_t flags) -> void {
    const flecs::entity ent {scene::get_active(), id};
    if (!ent.is_alive()) [[unlikely]] return;
    ent.get_mut<com::metadata>()->flags = flags;
}

