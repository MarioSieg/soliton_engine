// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../base.hpp"

namespace lu::com {
    struct transform final {
        XMFLOAT4 position; // only xyz is used, w is padding for SIMD
        XMFLOAT4 rotation;
        XMFLOAT4 scale; // only xyz is used, w is padding for SIMD

        transform() noexcept;

        [[nodiscard]] auto XM_CALLCONV compute_matrix() const noexcept -> XMMATRIX;
        [[nodiscard]] auto XM_CALLCONV forward_vec() const noexcept -> XMVECTOR;
        [[nodiscard]] auto XM_CALLCONV backward_vec() const noexcept -> XMVECTOR;
        [[nodiscard]] auto XM_CALLCONV up_vec() const noexcept -> XMVECTOR;
        [[nodiscard]] auto XM_CALLCONV down_vec() const noexcept -> XMVECTOR;
        [[nodiscard]] auto XM_CALLCONV left_vec() const noexcept -> XMVECTOR;
        [[nodiscard]] auto XM_CALLCONV right_vec() const noexcept -> XMVECTOR;
    };
}
