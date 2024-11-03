// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../base.hpp"

namespace lu::com {
    struct transform;

    struct camera final {
        float fov = 60.0f;
        bool auto_viewport = true;
        XMFLOAT2 z_near_far {0.1f, 1000.0f};
        XMFLOAT2 viewport;
        XMFLOAT3 clear_color;

        camera() noexcept;

        [[nodiscard]] static auto XM_CALLCONV compute_view(const transform& transform) noexcept -> XMMATRIX;
        [[nodiscard]] auto XM_CALLCONV compute_projection() const noexcept -> XMMATRIX;
    };
}
