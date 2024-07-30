// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"
#include "../../core/kernel.hpp"

LUA_INTEROP_API auto __lu_get_delta_time() -> double {
    return kernel::get().get_delta_time();
}
