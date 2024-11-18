// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "../core/core.hpp"
#include "layerdef.hpp"

namespace soliton::physics {
    /// Class that determines if an object layer can collide with a broadphase layer
    class broad_phase_layer_filter final : public JPH::ObjectVsBroadPhaseLayerFilter {
    public:
        virtual auto ShouldCollide(JPH::ObjectLayer layer1, JPH::BroadPhaseLayer layer2) const -> bool override;
    };
}
