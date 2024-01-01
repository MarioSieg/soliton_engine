// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../api_prelude.hpp"
#include "../../core/kernel.hpp"

LUA_INTEROP_API auto __lu_get_delta_time() -> double {
    return kernel::get_delta_time();
}
