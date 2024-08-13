// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <Jolt/Jolt.h>

namespace lu::physics::layers {
    constexpr JPH::ObjectLayer k_static {0}; // non moving
    constexpr JPH::ObjectLayer k_dynamic {1}; // moving
    constexpr std::uint32_t k_num {2};

    namespace broadphase {
        constexpr JPH::BroadPhaseLayer k_static {0}; // non moving
        constexpr JPH::BroadPhaseLayer k_dynamic {1}; // moving
        constexpr std::uint32_t k_num {2};
    }
}
