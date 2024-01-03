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

LUA_INTEROP_API auto __lu_entity_set_pos(const double id, const double x, const double y, const double z) {
    const entity ent = entity_from_id(id);
    if (auto* transform = ent.get_mut<c_transform>(); transform != nullptr) [[likely]] {
        transform->position = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z)
        };
    }
}

LUA_INTEROP_API auto __lu_entity_get_pos(const double id) -> lvec3 {
    const entity ent = entity_from_id(id);
    if (const auto* transform = ent.get<const c_transform>(); transform) [[likely]] {
        return lvec3{transform->position};
    }
    return {};
}
