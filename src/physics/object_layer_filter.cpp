// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "object_layer_filter.hpp"

namespace lu::physics {
    auto object_layer_filter::ShouldCollide(const JPH::ObjectLayer layer1, const JPH::ObjectLayer layer2) const -> bool {
        switch (layer1) {
            case layers::k_static: return layer2 == layers::k_dynamic; // Non moving only collides with moving
            case layers::k_dynamic: return true; // Moving collides with everything
            default: return false;
        }
    }
}
