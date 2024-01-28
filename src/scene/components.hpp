// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "base.hpp"

namespace graphics {
    class mesh;
}

struct c_metadata final {
    std::string name {};
};

struct c_transform final {
    XMFLOAT4 position;
    XMFLOAT4 rotation;
    XMFLOAT4 scale;

    c_transform() noexcept {
        XMStoreFloat4(&this->position, XMVectorZero());
        XMStoreFloat4(&this->rotation, XMQuaternionIdentity());
        XMStoreFloat4(&this->scale, XMVectorSplatOne());
    }

    [[nodiscard]] auto XM_CALLCONV compute_matrix() const noexcept -> XMMATRIX {
        const XMVECTOR zero { XMVectorZero() };
        return XMMatrixTransformation(
            zero,
            XMQuaternionIdentity(),
            XMLoadFloat4(&this->scale),
            zero,
            XMLoadFloat4(&this->rotation),
            XMLoadFloat4(&this->position)
        );
    }
    [[nodiscard]] auto XM_CALLCONV forward_vec() const noexcept -> XMVECTOR {
        return XMVector3Transform(XMVectorSet(.0F, .0F, 1.F, .0F), XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
    }
    [[nodiscard]] auto XM_CALLCONV backward_vec() const noexcept -> XMVECTOR {
        return XMVector3Transform(XMVectorSet(.0F, .0F, -1.F, .0F), XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
    }
    [[nodiscard]] auto XM_CALLCONV up_vec() const noexcept -> XMVECTOR {
        return XMVector3Transform(XMVectorSet(.0F, 1.F, 0.F, .0F), XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
    }
    [[nodiscard]] auto XM_CALLCONV down_vec() const noexcept -> XMVECTOR {
        return XMVector3Transform(XMVectorSet(.0F, -1.F, 0.F, .0F), XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
    }
    [[nodiscard]] auto XM_CALLCONV left_vec() const noexcept -> XMVECTOR {
        return XMVector3Transform(XMVectorSet(-1.0F, 0.F, 0.F, .0F), XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
    }
    [[nodiscard]] auto XM_CALLCONV right_vec() const noexcept -> XMVECTOR {
        return XMVector3Transform(XMVectorSet(1.0F, 0.F, 0.F, .0F), XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
    }
};

struct c_camera final {
    float fov = 60.0f;
    float z_clip_near = 0.1f;
    float z_clip_far = 1000.0f;
    bool auto_viewport = true;
    XMFLOAT2 viewport = {};

    static inline entity active_camera = entity::null(); // main camera, resetted every frame

   [[nodiscard]] static auto XM_CALLCONV compute_view(const c_transform& transform) noexcept -> XMMATRIX {
       const XMVECTOR eyePos { XMLoadFloat4(&transform.position) };
       const XMVECTOR focusPos { XMVectorAdd(eyePos, transform.forward_vec()) };
       return XMMatrixLookAtLH(eyePos, focusPos, XMVectorSet(.0F, 1.F, .0F, .0F));
   }

   [[nodiscard]] auto XM_CALLCONV compute_projection() const noexcept -> XMMATRIX {
       const float aspect = viewport.x/viewport.y;
       return XMMatrixPerspectiveFovLH(XMConvertToRadians(this->fov), aspect, this->z_clip_near, this->z_clip_far);
   }
};

struct render_flags final {
    enum $ {
        none = 0,
        skip_rendering = 1 << 0,
        skip_frustum_culling = 1 << 1,
    };
};

struct c_mesh_renderer final {
    graphics::mesh* mesh = nullptr;
    std::underlying_type_t<render_flags::$> flags = render_flags::none;
};
