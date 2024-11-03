// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "../core/core.hpp"
#include "layerdef.hpp"

namespace lu::physics {
    /// Class that determines if two object layers can collide
    class object_layer_filter final : public JPH::ObjectLayerPairFilter {
    public:
        virtual auto ShouldCollide(JPH::ObjectLayer layer1, JPH::ObjectLayer layer2) const -> bool override;
    };
}
