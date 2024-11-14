// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"
#include "../../../../scene/scene_import.hpp"

LUA_INTEROP_API auto __lu_scene_create(const char* name) -> int {
    panic_assert(name != nullptr);
    // pass empty file name to NOT load a file
    auto scene = eastl::make_unique<class scene>(name);
    int id = static_cast<int>(eastl::bit_cast<std::uintptr_t>(scene.get()));
    scene_mgr::set_active(std::move(scene));
    return id;
}

LUA_INTEROP_API auto __lu_scene_import(const char* name, const char* file, const std::uint32_t load_flags) -> int {
    auto scene = import_from_file(file, load_flags);
    scene->properties.name = name;
    int id = static_cast<int>(eastl::bit_cast<std::uintptr_t>(scene.get()));
    scene_mgr::set_active(std::move(scene));
    return id;
}

LUA_INTEROP_API auto __lu_scene_tick() -> void {
    scene_mgr::active().on_tick();
}

LUA_INTEROP_API auto __lu_scene_spawn_entity(const char* const name) -> lua_entity_id {
    return eastl::bit_cast<lua_entity_id>(scene_mgr::active().spawn(name).raw_id());
}

LUA_INTEROP_API auto __lu_scene_despawn_entity(const lua_entity_id id) -> void {
    const flecs::entity ent {scene_mgr::active(), eastl::bit_cast<flecs::id_t>(id)};
    ent.destruct();
}

LUA_INTEROP_API auto __lu_scene_get_entity_by_name(const char* const name) -> lua_entity_id {
    const flecs::entity entity = scene_mgr::active().lookup(name);
    if (!entity) [[unlikely]] {
        return 0;
    }
    return eastl::bit_cast<lua_entity_id>(entity.raw_id());
}

LUA_INTEROP_API auto __lu_scene_set_active_camera_entity(const lua_entity_id id) -> void {
    const flecs::entity ent {scene_mgr::active(), eastl::bit_cast<flecs::id_t>(id)};
    scene_mgr::active().active_camera = ent;
}

LUA_INTEROP_API auto __lu_scene_get_active_camera_entity() -> lua_entity_id {
    return eastl::bit_cast<lua_entity_id>(scene_mgr::active().active_camera.raw_id());
}

struct scene_iter_context final {
    flecs::query<const com::metadata> query {};
    scene* ref {};
    eastl::vector<flecs::id_t> data {};
};

static eastl::optional<scene_iter_context> s_scene_iter_context {};

LUA_INTEROP_API auto __lu_scene_full_entity_query_start() -> void {
    s_scene_iter_context.emplace();
    auto& ctx = s_scene_iter_context.value();
    scene& active = scene_mgr::active();
    if (&active != ctx.ref || !ctx.query) {
        ctx.ref = &active;
        ctx.query = active.query<const com::metadata>();
    }
    ctx.query.each([](const flecs::entity& e, const com::metadata& m) {
        s_scene_iter_context->data.emplace_back(e);
    });
}

LUA_INTEROP_API auto __lu_scene_full_entity_query_next_table() -> std::int32_t {
    auto& ctx = s_scene_iter_context;
    return ctx ? static_cast<std::int32_t>(ctx->data.size()) : 0;
}

LUA_INTEROP_API auto __lu_scene_full_entity_query_get(const std::int32_t i) -> lua_entity_id {
    auto& ctx = s_scene_iter_context;
    return ctx && !ctx->data.empty() ? eastl::bit_cast<lua_entity_id>(ctx->data[std::clamp<std::size_t>(i, 0, ctx->data.size())]) : 0.0;
}

LUA_INTEROP_API auto __lu_scene_full_entity_query_end() -> void {
    auto& ctx = s_scene_iter_context;
    if (ctx) {
        ctx.reset();
    }
}

LUA_INTEROP_API auto __lu_scene_set_sun_dir(const lua_vec3 sun_dir) -> void {
    scene_mgr::active().properties.environment.sun_dir = sun_dir;
}

LUA_INTEROP_API auto __lu_scene_set_sun_color(const lua_vec3 sun_color) -> void {
    scene_mgr::active().properties.environment.sun_color = sun_color;
}

LUA_INTEROP_API auto __lu_scene_set_ambient_color(const lua_vec3 ambient_color) -> void {
    scene_mgr::active().properties.environment.ambient_color = ambient_color;
}
