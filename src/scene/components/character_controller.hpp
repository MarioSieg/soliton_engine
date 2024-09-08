// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../base.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Character/Character.h>

namespace lu::com {
    struct character_controller final {
        JPH::Ref<JPH::Character> phys_character {};
    };
}
