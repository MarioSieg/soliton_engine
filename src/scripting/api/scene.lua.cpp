// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"

LUA_INTEROP_API auto __lu_scene_new(const char* name, const char* file, const double scale) -> std::uint32_t {
    std::string sname {}, sfile {};
    if (name) sname = name;
    if (file) sfile = file;
    scene::new_active(std::move(sname), std::move(sfile), static_cast<float>(scale));
    return scene::get_active()->id;
}

LUA_INTEROP_API auto __lu_scene_tick() -> void {
    if (!scene::get_active()) [[unlikely]]
        return;
    scene::get_active()->on_tick();
}

LUA_INTEROP_API auto __lu_scene_start() -> void {
    if (!scene::get_active()) [[unlikely]]
        return;
    scene::get_active()->on_start();
}

LUA_INTEROP_API auto __lu_scene_spawn_entity(const char* const name) -> lua_entity {
    if (!scene::get_active()) [[unlikely]]
        return scene::k_invalid_entity;
    lua_entity id = 0;
    scene::get_active()->spawn(name, &id);
    return id;
}

LUA_INTEROP_API auto __lu_scene_get_entity_by_name(const char* const name) -> lua_entity {
    const auto& active = scene::get_active();
    if (!active) [[unlikely]]
       return scene::k_invalid_entity;
    const flecs::entity entity = scene::get_active()->lookup(name);
    if (!entity) [[unlikely]]
        return scene::k_invalid_entity;
    const std::span<const flecs::entity> entities = active->get_eitbl();
    const auto entry = std::find(entities.begin(), entities.end(), entity); // TODO: Smarter way to handle this?
    if (entry != entities.end()) [[likely]] {
        const std::ptrdiff_t id = std::distance(entities.begin(), entry);
        passert(id <= scene::k_max_entities);
        return static_cast<lua_entity>(id);
    }
    return scene::k_invalid_entity;
}

