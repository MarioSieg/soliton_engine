// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "../core/core.hpp"
#include "layerdef.hpp"

namespace soliton::physics {
    // BroadPhaseLayerInterface implementation
    // This defines a mapping between object and broadphase layers.
    class broad_phase_layer final : public JPH::BroadPhaseLayerInterface {
    public:
        broad_phase_layer() noexcept;
        virtual auto GetNumBroadPhaseLayers() const -> JPH::uint override;
        virtual auto GetBroadPhaseLayer(JPH::ObjectLayer layer) const -> JPH::BroadPhaseLayer override;
        #if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
            virtual auto GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const -> const char* override;
        #endif
    private:
        eastl::array<JPH::BroadPhaseLayer, layers::k_num> m_broad_phase {};
    };
}
