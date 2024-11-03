// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "scene_mgr.hpp"

namespace lu::scene_mgr {
    static eastl::unique_ptr<scene> s_active_scene {};

    auto active() -> scene& {
        panic_assert(s_active_scene != nullptr);
        return *s_active_scene;
    }

    auto set_active(eastl::unique_ptr<scene>&& new_scene) -> void {
        s_active_scene.reset();
        if (!new_scene) return; // Just reset scene
        s_active_scene = std::move(new_scene);
        s_active_scene->on_start(); // Start the new scene
    }
}
