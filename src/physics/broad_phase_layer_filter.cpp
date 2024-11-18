// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "broad_phase_layer_filter.hpp"

namespace soliton::physics {
    auto broad_phase_layer_filter::ShouldCollide(const JPH::ObjectLayer layer1, const JPH::BroadPhaseLayer layer2) const -> bool {
        switch (layer1) {
            case layers::k_static: return layer2 == layers::broadphase::k_dynamic;
            case layers::k_dynamic: return true;
            default: return false;
        }
    }
}
