// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../_prelude.hpp"
#include "../../../physics/physics_subsystem.hpp"

impl_component_core(transform)

LUA_INTEROP_API auto __lu_com_transform_set_pos(const lua_entity_id id, const double x, const double y, const double z) -> void {
    std::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return; }
    // If the entity has a rigidbody component, set the position of the rigidbody todo fix
    // as the transform position will be overwritten by the physics system
    if (const auto* rigidbody = ent->get<const com::rigidbody>(); rigidbody) {
        auto& bi = physics::physics_subsystem::get_physics_system().GetBodyInterface();
        bi.SetPosition(rigidbody->phys_body, JPH::Vec3{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}, JPH::EActivation::Activate);
    } else if (const auto* cc = ent->get<const com::character_controller>(); cc) {
        cc->phys_character->SetPosition(JPH::Vec3{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}, JPH::EActivation::Activate);
    } else if (auto* transform = ent->get_mut<com::transform>(); transform) [[likely]] {
        transform->position = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z),
            .0f
        };
    }
}

LUA_INTEROP_API auto __lu_com_transform_get_pos(const lua_entity_id id) -> lua_vec3 {
    std::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return {}; }
    if (const auto* transform = ent->get_mut<const com::transform>(); transform) [[likely]] {
        return transform->position;
    }
    return {};
}

LUA_INTEROP_API auto __lu_com_transform_set_rot(const lua_entity_id id, const double x, const double y, const double z, const double w) -> void {
    std::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return; }
    // If the entity has a rigidbody component, set the position of the rigidbody
    // as the transform position will be overwritten by the physics system
    if (const auto* rigidbody = ent->get<const com::rigidbody>(); rigidbody) {
        auto& bi = physics::physics_subsystem::get_physics_system().GetBodyInterface();
        bi.SetRotation(rigidbody->phys_body, JPH::Quat{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), static_cast<float>(w)}, JPH::EActivation::Activate);
    } else if (const auto* cc = ent->get<const com::character_controller>(); cc) {
        cc->phys_character->SetRotation(JPH::Quat{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), static_cast<float>(w)}, JPH::EActivation::Activate);
    } else if (auto* transform = ent->get_mut<com::transform>(); transform) [[likely]] {
        transform->rotation = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z),
            static_cast<float>(w)
        };
    }
}

LUA_INTEROP_API auto __lu_com_transform_get_rot(const lua_entity_id id) -> lua_vec4 {
    std::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return {}; }
    if (const auto* transform = ent->get_mut<const com::transform>(); transform) [[likely]] {
        return transform->rotation;
    }
    return {};
}

LUA_INTEROP_API auto __lu_com_transform_set_scale(const lua_entity_id id, const double x, const double y, const double z) -> void {
    std::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return; }
    if (auto* transform = ent->get_mut<com::transform>(); transform) [[likely]] {
        transform->scale = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z),
            .0f
        };
    }
}

LUA_INTEROP_API auto __lu_com_transform_get_scale(const lua_entity_id id) -> lua_vec3 {
    std::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return {}; }
    if (const auto* transform = ent->get_mut<const com::transform>(); transform) [[likely]] {
        return transform->scale;
    }
    return {};
}

LUA_INTEROP_API auto __lu_com_transform_get_forward(const lua_entity_id id) -> lua_vec3 {
    std::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return {}; }
    if (const auto* transform = ent->get_mut<const com::transform>(); transform) [[likely]] {
        DirectX::XMFLOAT3A tmp;
        DirectX::XMStoreFloat3A(&tmp, transform->forward_vec());
        return tmp;
    }
    return {};
}

LUA_INTEROP_API auto __lu_com_transform_get_backward(const lua_entity_id id) -> lua_vec3 {
    std::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return {}; }
    if (const auto* transform = ent->get_mut<const com::transform>(); transform) [[likely]] {
        DirectX::XMFLOAT3A tmp;
        DirectX::XMStoreFloat3A(&tmp, transform->backward_vec());
        return tmp;
    }
    return {};
}

LUA_INTEROP_API auto __lu_com_transform_get_up(const lua_entity_id id) -> lua_vec3 {
    std::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return {}; }
    if (const auto* transform = ent->get_mut<const com::transform>(); transform) [[likely]] {
        DirectX::XMFLOAT3A tmp;
        DirectX::XMStoreFloat3A(&tmp, transform->up_vec());
        return tmp;
    }
    return {};
}

LUA_INTEROP_API auto __lu_com_transform_get_down(const lua_entity_id id) -> lua_vec3 {
    std::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return {}; }
    if (const auto* transform = ent->get_mut<const com::transform>(); transform) [[likely]] {
        DirectX::XMFLOAT3A tmp;
        DirectX::XMStoreFloat3A(&tmp, transform->down_vec());
        return tmp;
    }
    return {};
}

LUA_INTEROP_API auto __lu_com_transform_get_right(const lua_entity_id id) -> lua_vec3 {
    std::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return {}; }
    if (const auto* transform = ent->get_mut<const com::transform>(); transform) [[likely]] {
        DirectX::XMFLOAT3A tmp;
        DirectX::XMStoreFloat3A(&tmp, transform->right_vec());
        return tmp;
    }
    return {};
}

LUA_INTEROP_API auto __lu_com_transform_get_left(const lua_entity_id id) -> lua_vec3 {
    std::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return {}; }
    if (const auto* transform = ent->get_mut<const com::transform>(); transform) [[likely]] {
        DirectX::XMFLOAT3A tmp;
        DirectX::XMStoreFloat3A(&tmp, transform->left_vec());
        return tmp;
    }
    return {};
}

