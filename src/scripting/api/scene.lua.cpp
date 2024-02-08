// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"

LUA_INTEROP_API auto __lu_scene_new(const char* name, const char* file, const double scale) -> std::uint32_t {
    std::string sname {}, sfile {};
    if (name) {
        sname = name;
    }
    if (file) {
        sfile = file;
    }
    scene::new_active(std::move(sname), std::move(sfile), static_cast<float>(scale));
    return scene::get_active().id;
}

LUA_INTEROP_API auto __lu_scene_tick() -> void {
    scene::get_active().on_tick();
}

LUA_INTEROP_API auto __lu_scene_start() -> void {
    scene::get_active().on_start();
}

LUA_INTEROP_API auto __lu_scene_spawn_entity(const char* const name) -> flecs::id_t {
    return scene::get_active().spawn(name).raw_id();
}

LUA_INTEROP_API auto __lu_scene_get_entity_by_name(const char* const name) -> flecs::id_t {
    const flecs::entity entity = scene::get_active().lookup(name);
    if (!entity) [[unlikely]] {
        return 0;
    }
    return entity.raw_id();
}

