// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "../_prelude.hpp"

impl_component_core(character_controller)

LUA_INTEROP_API auto __lu_com_character_controller_get_linear_velocity(const lua_entity_id id) -> lua_vec3 {
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return {}; }
    if (const auto* cc = ent->get<com::character_controller>(); cc) [[likely]] {
        return cc->phys_character->GetLinearVelocity();
    }
    return {};
}

LUA_INTEROP_API auto __lu_com_character_controller_set_linear_velocity(const lua_entity_id id, const lua_vec3 velocity) -> void {
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return; }
    if (auto* cc = ent->get_mut<com::character_controller>(); cc) [[likely]] {
        cc->phys_character->SetLinearVelocity(velocity);
    }
}

LUA_INTEROP_API auto __lu_com_character_controller_get_ground_state(const lua_entity_id id) -> int {
    static_assert(std::is_same_v<std::underlying_type_t<JPH::CharacterBase::EGroundState>, int>);
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return false; }
    if (const auto* cc = ent->get<com::character_controller>(); cc) [[likely]] {
        return static_cast<int>(cc->phys_character->GetGroundState());
    }
    return static_cast<int>(JPH::CharacterBase::EGroundState::NotSupported);
}

LUA_INTEROP_API auto __lu_com_character_controller_get_ground_normal(const lua_entity_id id) -> lua_vec3 {
    eastl::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return {}; }
    if (const auto* cc = ent->get<com::character_controller>(); cc) [[likely]] {
        return cc->phys_character->GetGroundNormal();
    }
    return {};
}
