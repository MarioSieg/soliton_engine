// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/core.hpp"

namespace lu {
    // Contains all editable properties of a scene.
    struct scene_properties final {
        struct {
            DirectX::XMFLOAT4 sun_dir {0.0f, -1.0f, 0.0f, 1.0f};
            DirectX::XMFLOAT4 sun_color {1.0f, 0.0f, 0.0f, 1.0f};
        } environment {};
    };
}
