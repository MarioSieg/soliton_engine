// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "base.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Character/Character.h>

namespace graphics {
    class mesh;
    class texture;
    class material;
}

namespace com {
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
        DirectX::XMFLOAT4 position; // only xyz is used, w is padding for SIMD
        DirectX::XMFLOAT4 rotation;
        DirectX::XMFLOAT4 scale; // only xyz is used, w is padding for SIMD

        transform() noexcept {
            XMStoreFloat4(&this->position, DirectX::XMVectorZero());
            XMStoreFloat4(&this->rotation, DirectX::XMQuaternionIdentity());
            XMStoreFloat4(&this->scale, DirectX::XMVectorSplatOne());
        }

        [[nodiscard]] auto XM_CALLCONV compute_matrix() const noexcept -> DirectX::XMMATRIX {
            const DirectX::XMVECTOR zero { DirectX::XMVectorZero() };
            return DirectX::XMMatrixTransformation(
                zero,
                DirectX::XMQuaternionIdentity(),
                XMLoadFloat4(&this->scale),
                zero,
                XMLoadFloat4(&this->rotation),
                XMLoadFloat4(&this->position)
            );
        }
        [[nodiscard]] auto XM_CALLCONV forward_vec() const noexcept -> DirectX::XMVECTOR {
            return DirectX::XMVector3Transform(DirectX::XMVectorSet(.0F, .0F, 1.F, .0F), DirectX::XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
        }
        [[nodiscard]] auto XM_CALLCONV backward_vec() const noexcept -> DirectX::XMVECTOR {
            return DirectX::XMVector3Transform(DirectX::XMVectorSet(.0F, .0F, -1.F, .0F), DirectX::XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
        }
        [[nodiscard]] auto XM_CALLCONV up_vec() const noexcept -> DirectX::XMVECTOR {
            return XMVector3Transform(DirectX::XMVectorSet(.0F, 1.F, 0.F, .0F), DirectX::XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
        }
        [[nodiscard]] auto XM_CALLCONV down_vec() const noexcept -> DirectX::XMVECTOR {
            return DirectX::XMVector3Transform(DirectX::XMVectorSet(.0F, -1.F, 0.F, .0F), DirectX::XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
        }
        [[nodiscard]] auto XM_CALLCONV left_vec() const noexcept -> DirectX::XMVECTOR {
            return XMVector3Transform(DirectX::XMVectorSet(-1.0F, 0.F, 0.F, .0F), DirectX::XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
        }
        [[nodiscard]] auto XM_CALLCONV right_vec() const noexcept -> DirectX::XMVECTOR {
            return XMVector3Transform(DirectX::XMVectorSet(1.0F, 0.F, 0.F, .0F), DirectX::XMMatrixRotationQuaternion(XMLoadFloat4(&this->rotation)));
        }
    };

    struct camera final {
        float fov = 60.0f;
        float z_clip_near = 0.1f;
        float z_clip_far = 1000.0f;
        bool auto_viewport = true;
        DirectX::XMFLOAT2 viewport;
        DirectX::XMFLOAT3 clear_color;

        camera() noexcept {
            DirectX::XMStoreFloat2(&viewport, DirectX::XMVectorZero());
            DirectX::XMStoreFloat3(&clear_color, DirectX::XMVectorSplatOne());
        }

        static inline flecs::entity active_camera = flecs::entity::null(); // main camera, resetted every frame

       [[nodiscard]] static auto XM_CALLCONV compute_view(const transform& transform) noexcept -> DirectX::XMMATRIX {
           const DirectX::XMVECTOR eyePos { DirectX::XMLoadFloat4(&transform.position) };
           const DirectX::XMVECTOR focusPos { DirectX::XMVectorAdd(eyePos, transform.forward_vec()) };
           return DirectX::XMMatrixLookAtLH(eyePos, focusPos, DirectX::XMVectorSet(.0F, 1.F, .0F, .0F));
       }

       [[nodiscard]] auto XM_CALLCONV compute_projection() const noexcept -> DirectX::XMMATRIX {
           const float aspect = viewport.x / viewport.y;
           return DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(this->fov), aspect, this->z_clip_near, this->z_clip_far);
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
        std::vector<graphics::mesh*> meshes {};
        std::vector<graphics::material*> materials {};
        std::underlying_type_t<render_flags::$> flags = render_flags::none;
    };

    struct rigidbody final {
        JPH::BodyID body_id {};
    };

    struct character_controller final {
        JPH::Ref<JPH::Character> characer {};
    };
}
