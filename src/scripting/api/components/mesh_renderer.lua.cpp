// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../_prelude.hpp"

impl_component_core(mesh_renderer)

LUA_INTEROP_API auto __lu_com_mesh_renderer_get_flags(const lua_entity_id id) -> std::uint32_t {
    std::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return 0; }
    if (const auto* const mesh_renderer = ent->get<const com::mesh_renderer>(); mesh_renderer) [[likely]] {
        return mesh_renderer->flags;
    }
    return 0;
}

LUA_INTEROP_API auto __lu_com_mesh_renderer_set_flags(const lua_entity_id id, const std::uint32_t flags) -> void {
    std::optional<flecs::entity> ent {resolve_entity(id)};
    if (!ent) [[unlikely]] { return; }
    if (auto* const mesh_renderer = ent->get_mut<com::mesh_renderer>(); mesh_renderer) [[likely]] {
        mesh_renderer->flags = flags;
    }
}