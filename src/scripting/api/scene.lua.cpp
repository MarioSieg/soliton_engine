// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../api_prelude.hpp"

LUA_INTEROP_API auto __lu_scene_new() -> std::uint32_t {
    scene::new_active();
    return scene::get_active()->id;
}

LUA_INTEROP_API auto __lu_scene_tick() -> void {
    if (scene::get_active() == nullptr) [[unlikely]] return;
    scene::get_active()->on_tick();
}

LUA_INTEROP_API auto __lu_scene_start() -> void {
    if (scene::get_active() == nullptr) [[unlikely]] return;
    scene::get_active()->on_start();
}

LUA_INTEROP_API auto __lu_scene_spawn_entity(const char* const name) -> double {
    if (scene::get_active() == nullptr) [[unlikely]] {
        return entity_id_to_double(0);
    }
    const entity entity = scene::get_active()->spawn(name);
    const double d = entity_id_to_double(entity.id());
    return d;
}

LUA_INTEROP_API auto __lu_scene_get_entity_by_name(const char* const name) -> double {
    if (scene::get_active() == nullptr) [[unlikely]] {
        return entity_id_to_double(0);
    }
    const entity entity = scene::get_active()->lookup(name);
    const double d = entity_id_to_double(entity.id());
    return d;
}

