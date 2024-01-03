// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../api_prelude.hpp"

LUA_INTEROP_API auto __lu_entity_is_valid(const double id) -> bool {
    const entity ent = entity_from_id(id);
    return ent.is_valid();
}

LUA_INTEROP_API auto __lu_entity_is_alive(const double id) -> bool {
    const entity ent = entity_from_id(id);
    return ent.is_alive();
}

