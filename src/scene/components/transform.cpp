// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "transform.hpp"

namespace lu::com {
    transform::transform() noexcept {
        XMStoreFloat4(&this->position, XMVectorZero());
        XMStoreFloat4(&this->rotation, XMQuaternionIdentity());
        XMStoreFloat4(&this->scale, XMVectorSplatOne());
    }

    auto transform::compute_matrix() const noexcept -> XMMATRIX {
        return XMMatrixTransformation(
            g_XMZero,
            XMQuaternionIdentity(),
            XMLoadFloat4(&this->scale),
            g_XMZero,
            XMLoadFloat4(&this->rotation),
            XMLoadFloat4(&this->position)
        );
    }

    auto transform::forward_vec() const noexcept -> XMVECTOR {
        return XMVector3Transform(XMVectorSet(.0F, .0F, 1.F, .0F), XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
    }

    auto transform::backward_vec() const noexcept -> XMVECTOR {
        return XMVector3Transform(XMVectorSet(.0F, .0F, -1.F, .0F), XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
    }

    auto transform::up_vec() const noexcept -> XMVECTOR {
        return XMVector3Transform(XMVectorSet(.0F, 1.F, 0.F, .0F), XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
    }

    auto transform::down_vec() const noexcept -> XMVECTOR {
        return XMVector3Transform(XMVectorSet(.0F, -1.F, 0.F, .0F), XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
    }

    auto transform::left_vec() const noexcept -> XMVECTOR {
        return XMVector3Transform(XMVectorSet(-1.0F, 0.F, 0.F, .0F), XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
    }

    auto transform::right_vec() const noexcept -> XMVECTOR {
        return XMVector3Transform(XMVectorSet(1.0F, 0.F, 0.F, .0F), XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
    }
}
