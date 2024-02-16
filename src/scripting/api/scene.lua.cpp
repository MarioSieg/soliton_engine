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

// TODO: Convert to FLECS C++ API

static struct {
    flecs::iter_t iter{};
    scene* assoc {};
} s_scene_iter_context;

LUA_INTEROP_API auto __lu_scene_full_entity_query_start() -> void {
    scene& active = scene::get_active();
    s_scene_iter_context.assoc = &active;
    std::array<flecs::iter_t, 2> iters {};
    // search for objects with com::metadata assigned
    ecs_term_t term = {
        .id = flecs::type_id<com::metadata>(),
        .inout = EcsIn,
        .oper = EcsOptional
    };
    ecs_iter_poly(active.m_world, active.m_world, iters.data(), &term);
    s_scene_iter_context.iter = iters[0];
    ecs_iter_fini(&iters[1]);
}

LUA_INTEROP_API auto __lu_scene_full_entity_query_next_table() -> std::int32_t {
    passert(&scene::get_active() == s_scene_iter_context.assoc);
    return ecs_iter_next(&s_scene_iter_context.iter) ? s_scene_iter_context.iter.count : 0;
}

LUA_INTEROP_API auto __lu_scene_full_entity_query_get(const std::int32_t i) -> flecs::id_t {
    passert(&scene::get_active() == s_scene_iter_context.assoc);
    return s_scene_iter_context.iter.entities[i];
}

LUA_INTEROP_API auto __lu_scene_full_entity_query_end() -> void {
    passert(&scene::get_active() == s_scene_iter_context.assoc);
    ecs_iter_fini(&s_scene_iter_context.iter);
    s_scene_iter_context.iter = {};
    s_scene_iter_context.assoc = nullptr;
}
