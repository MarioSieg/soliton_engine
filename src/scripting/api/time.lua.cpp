// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../api_prelude.hpp"

LUA_INTEROP_API auto __lu_get_delta_time() -> double {
    static auto last = std::chrono::high_resolution_clock::now();
    const auto now = std::chrono::high_resolution_clock::now();
    double delta_t = std::chrono::duration_cast<std::chrono::duration<double>>(now - last).count();
    last = now;
    return delta_t;
}
