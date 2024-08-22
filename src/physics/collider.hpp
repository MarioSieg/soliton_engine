// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/core.hpp"
#include "../graphics/vertex.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

namespace lu::physics {
    class collider final : public no_copy {
    public:
        enum class shape_type {
            box,
            sphere,
            cylinder,
            capsule,
            convex,
            mesh
        };

        [[nodiscard]] static auto new_box(const DirectX::XMFLOAT3A& half_extent) -> collider;
        [[nodiscard]] static auto new_sphere(float radius) -> collider;
        [[nodiscard]] static auto new_cylinder(float half_height, float radius) -> collider;
        [[nodiscard]] static auto new_capsule(float half_height, float radius) -> collider;
        [[nodiscard]] static auto new_mesh(eastl::span<const graphics::vertex> vertices, eastl::span<const graphics::index> indices) -> collider;

        [[nodiscard]] auto type() const noexcept -> shape_type { return m_shape_type; }
        [[nodiscard]] auto data() const noexcept -> JPH::Shape* { return &*m_shape; }

    private:
        explicit collider(const shape_type type, JPH::Ref<JPH::Shape>&& shape) noexcept
            : m_shape_type{type}, m_shape{std::move(shape)} {}
        shape_type m_shape_type {};
        JPH::Ref<JPH::Shape> m_shape {};
    };
}
