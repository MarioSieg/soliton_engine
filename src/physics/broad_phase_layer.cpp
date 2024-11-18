// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "broad_phase_layer.hpp"

namespace soliton::physics {
    broad_phase_layer::broad_phase_layer() noexcept {
        // Create a mapping table from object to broad phase layer
        m_broad_phase[layers::k_static] = layers::broadphase::k_static;
        m_broad_phase[layers::k_dynamic] = layers::broadphase::k_dynamic;
    }

    auto broad_phase_layer::GetNumBroadPhaseLayers() const -> JPH::uint {
        return layers::k_num;
    }

    auto broad_phase_layer::GetBroadPhaseLayer(const JPH::ObjectLayer layer) const -> JPH::BroadPhaseLayer {
        JPH_ASSERT(layer < layers::k_num);
        return m_broad_phase[layer];
    }

    #if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        auto broad_phase_layer::GetBroadPhaseLayerName(const JPH::BroadPhaseLayer layer) const -> const char* {
            return "";
        }
    #endif
}
