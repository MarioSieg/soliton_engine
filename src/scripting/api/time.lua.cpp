// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"
#include "../../core/kernel.hpp"

#include <chrono>

using namespace std::chrono;

LUA_INTEROP_API auto __lu_get_delta_time() -> double {
    return kernel::get().get_delta_time();
}

LUA_INTEROP_API auto __lu_get_hpc_clock_now_ns() -> double {
    static const auto start_time = high_resolution_clock::now();
    return duration_cast<duration<double, std::nano>>(high_resolution_clock::now() - start_time).count();
}
