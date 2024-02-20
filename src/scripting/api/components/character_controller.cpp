// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../_prelude.hpp"

impl_component_core(character_controller)

LUA_INTEROP_API auto __lu_com_character_controller_get_linear_velocity(const flecs::id_t id) -> lua_vec3 {
    const flecs::entity ent {scene::get_active(), id};
    if (const auto* cc = ent.get<com::character_controller>(); cc) [[likely]] {
        return cc->characer->GetLinearVelocity();
    }
    return {};
}

LUA_INTEROP_API auto __lu_com_character_controller_set_linear_velocity(const flecs::id_t id, const lua_vec3 velocity) -> void {
    const flecs::entity ent {scene::get_active(), id};
    if (auto* cc = ent.get_mut<com::character_controller>(); cc) [[likely]] {
        cc->characer->SetLinearVelocity(velocity);
    }
}

LUA_INTEROP_API auto __lu_com_character_controller_get_ground_state(const flecs::id_t id) -> int {
    static_assert(std::is_same_v<std::underlying_type_t<JPH::CharacterBase::EGroundState>, int>);
    const flecs::entity ent {scene::get_active(), id};
    if (const auto* cc = ent.get<com::character_controller>(); cc) [[likely]] {
        return static_cast<int>(cc->characer->GetGroundState());
    }
    return static_cast<int>(JPH::CharacterBase::EGroundState::NotSupported);
}

LUA_INTEROP_API auto __lu_com_character_controller_get_ground_normal(const flecs::id_t id) -> lua_vec3 {
    const flecs::entity ent {scene::get_active(), id};
    if (const auto* cc = ent.get<com::character_controller>(); cc) [[likely]] {
        return cc->characer->GetGroundNormal();
    }
    return {};
}
