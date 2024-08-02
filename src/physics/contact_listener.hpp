// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "../core/core.hpp"
#include "layerdef.hpp"

namespace lu::physics {
    class contact_listener final : public JPH::ContactListener {
        virtual auto OnContactAdded(
            const JPH::Body& body1,
            const JPH::Body& body2,
            const JPH::ContactManifold& manifold,
            JPH::ContactSettings& io
        ) -> void override;
        virtual auto OnContactPersisted(
            const JPH::Body& body1,
            const JPH::Body& body2,
            const JPH::ContactManifold& manifold,
            JPH::ContactSettings& io
        ) -> void override;
    };
}
