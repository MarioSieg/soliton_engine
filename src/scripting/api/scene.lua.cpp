// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../api_prelude.hpp"
#include "../../scene/scene.hpp"

LUA_INTEROP_API auto __lu_scene_new() -> std::uint32_t {
    scene::new_active();
    return scene::get_active()->id;
}

LUA_INTEROP_API auto __lu_scene_tick() -> void {
    if (scene::get_active() != nullptr) [[likely]] {
        scene::get_active()->on_tick();
    }
}

LUA_INTEROP_API auto __lu_scene_start() -> void {
    if (scene::get_active() != nullptr) [[likely]] {
        scene::get_active()->on_start();
    }
}

