// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "camera.hpp"
#include "transform.hpp"

namespace soliton::com {
    camera::camera() noexcept {
        XMStoreFloat2(&viewport, XMVectorZero());
        XMStoreFloat3(&clear_color, XMVectorReplicate(0.05f));
    }

    auto camera::compute_view(const transform& transform) noexcept -> XMMATRIX {
        const XMVECTOR eye {XMLoadFloat4(&transform.position) };
        const XMVECTOR focal {XMVectorAdd(eye, transform.forward_vec()) };
        return XMMatrixLookAtLH(eye, focal, XMVectorSet(.0F, 1.F, .0F, .0F));
    }

    auto camera::compute_projection() const noexcept -> XMMATRIX {
        const float aspect = viewport.x / viewport.y;
        return XMMatrixPerspectiveFovLH(XMConvertToRadians(this->fov), aspect, this->z_near_far.x, this->z_near_far.y);
    }
}
