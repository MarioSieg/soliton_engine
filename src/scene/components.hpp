// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "base.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Character/Character.h>

namespace lu::graphics {
    class mesh;
    class texture;
    class material;
}

namespace lu::com {
    struct entity_flags final {
        enum $ : std::uint32_t {
            none = 0,
            hidden = 1 << 0,
            transient = 1 << 1,
            static_object = 1 << 2
        };
    };

    struct metadata final {
        std::underlying_type_t<entity_flags::$> flags = entity_flags::none;
    };

    struct transform final {
        XMFLOAT4 position; // only xyz is used, w is padding for SIMD
        XMFLOAT4 rotation;
        XMFLOAT4 scale; // only xyz is used, w is padding for SIMD

        transform() noexcept {
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

    struct camera final {
        float fov = 60.0f;
        float z_clip_near = 0.1f;
        float z_clip_far = 1000.0f;
        bool auto_viewport = true;
        XMFLOAT2 viewport;
        XMFLOAT3 clear_color;

        camera() noexcept {
            XMStoreFloat2(&viewport, XMVectorZero());
            XMStoreFloat3(&clear_color, XMVectorReplicate(0.05f));
        }

       [[nodiscard]] static auto XM_CALLCONV compute_view(const transform& transform) noexcept -> XMMATRIX {
           const XMVECTOR eye {XMLoadFloat4(&transform.position) };
           const XMVECTOR focal {XMVectorAdd(eye, transform.forward_vec()) };
           return XMMatrixLookAtLH(eye, focal, XMVectorSet(.0F, 1.F, .0F, .0F));
       }

       [[nodiscard]] auto XM_CALLCONV compute_projection() const noexcept -> XMMATRIX {
           const float aspect = viewport.x / viewport.y;
           return XMMatrixPerspectiveFovLH(XMConvertToRadians(this->fov), aspect, this->z_clip_near, this->z_clip_far);
       }
    };

    struct render_flags final {
        enum $ : std::uint32_t {
            none = 0,
            skip_rendering = 1 << 0,
            skip_frustum_culling = 1 << 1,
        };
    };

    struct mesh_renderer final {
        eastl::fixed_vector<graphics::mesh*, 4> meshes {};
        eastl::fixed_vector<graphics::material*, 4> materials {};
        std::underlying_type_t<render_flags::$> flags = render_flags::none;
    };

    struct rigidbody final {
        JPH::BodyID phys_body {};
    };

    struct character_controller final {
        JPH::Ref<JPH::Character> phys_character {};
    };
}
