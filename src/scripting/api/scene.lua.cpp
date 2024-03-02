// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"

LUA_INTEROP_API auto __lu_scene_import(const char* name, const char* file, const double scale, const std::uint32_t load_flags) -> std::uint32_t {
    std::string sname {}, sfile {};
    if (name) {
        sname = name;
    }
    if (file) {
        sfile = file;
    }
    scene::new_active(std::move(sname), std::move(sfile), static_cast<float>(scale), load_flags);
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

LUA_INTEROP_API auto __lu_scene_despawn_entity(const flecs::id_t id) -> void {
    const flecs::entity ent {scene::get_active(), id};
    ent.destruct();
}

LUA_INTEROP_API auto __lu_scene_get_entity_by_name(const char* const name) -> flecs::id_t {
    const flecs::entity entity = scene::get_active().lookup(name);
    if (!entity) [[unlikely]] {
        return 0;
    }
    return entity.raw_id();
}

LUA_INTEROP_API auto __lu_scene_set_active_camera_entity(const flecs::id_t id) -> void {
    const flecs::entity ent {scene::get_active(), id};
    scene::get_active().active_camera = ent;
}

LUA_INTEROP_API auto __lu_scene_get_active_camera_entity() -> flecs::id_t {
    return scene::get_active().active_camera.raw_id();
}

// TODO: Convert to FLECS C++ API

static struct {
    flecs::query<const com::metadata> query {};
    scene* ref {};
    std::vector<flecs::id_t> data {};
} s_scene_iter_context;

LUA_INTEROP_API auto __lu_scene_full_entity_query_start() -> void {
    auto& ctx = s_scene_iter_context;
    scene& active = scene::get_active();
    if (&active != ctx.ref || !ctx.query) {
        ctx.ref = &active;
        ctx.query = active.query<const com::metadata>();
    }
    ctx.query.each([](const flecs::entity& e, const com::metadata& m) {
        s_scene_iter_context.data.emplace_back(e);
    });
}

LUA_INTEROP_API auto __lu_scene_full_entity_query_next_table() -> std::int32_t {
    auto& ctx = s_scene_iter_context;
    return static_cast<std::int32_t>(ctx.data.size());
}

LUA_INTEROP_API auto __lu_scene_full_entity_query_get(const std::int32_t i) -> flecs::id_t {
    auto& ctx = s_scene_iter_context;
    return ctx.data[i];
}

LUA_INTEROP_API auto __lu_scene_full_entity_query_end() -> void {
    auto& ctx = s_scene_iter_context;
    ctx.data.clear();
}
