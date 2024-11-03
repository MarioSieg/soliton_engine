// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "scene.hpp"

namespace lu::scene_mgr {
    [[nodiscard]] extern auto active() -> scene&;
    extern auto set_active(eastl::unique_ptr<scene>&& new_scene) -> void;
}
