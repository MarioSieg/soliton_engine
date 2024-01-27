#pragma once

#include "../core/core.hpp"

#include "../ecs/flecs.h"

using namespace flecs;

#include "../math/DirectXMath.h"
#include "../math/DirectXCollision.h"

using namespace DirectX;

static_assert(sizeof(flecs::id_t) == 8);
