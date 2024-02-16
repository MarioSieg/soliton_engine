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
