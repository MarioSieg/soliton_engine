// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"

LUA_INTEROP_API auto __lu_scene_new() -> std::uint32_t {
    scene::new_active();
    return scene::get_active()->id;
}

LUA_INTEROP_API auto __lu_scene_tick() -> void {
    if (!scene::get_active()) [[unlikely]] return;
    scene::get_active()->on_tick();
}

LUA_INTEROP_API auto __lu_scene_start() -> void {
    if (!scene::get_active()) [[unlikely]] return;
    scene::get_active()->on_start();
}

LUA_INTEROP_API auto __lu_scene_spawn_entity(const char* const name) -> lua_entity_id {
    if (!scene::get_active()) [[unlikely]] {
        return {};
    }
    const entity entity = scene::get_active()->spawn(name);
    return lua_entity_id_from_entity_id(entity.id());
}

LUA_INTEROP_API auto __lu_scene_get_entity_by_name(const char* const name) -> lua_entity_id {
    log_info("Looking up entity by name: {}", name);
    if (!scene::get_active()) [[unlikely]] {
        return {};
    }
    const entity entity = scene::get_active()->lookup(name);
    return lua_entity_id_from_entity_id(entity.id());
}

