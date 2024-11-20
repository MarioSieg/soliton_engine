// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/core.hpp"

namespace soliton {
    // Contains all editable properties of a scene.
    struct scene_properties final {
        eastl::string name {};
        struct {
            XMFLOAT4 sun_dir {0.0f, -1.0f, 0.0f, 1.0f};
            XMFLOAT4 sun_color {1.0f, 1.0f, 1.0f, 1.0f};
            XMFLOAT4 ambient_color {1.0f, 1.0f, 1.0f, 1.0f};
            float time {12.0f};
            float sky_turbidity {2.15f};
        } environment {};
    };
}
