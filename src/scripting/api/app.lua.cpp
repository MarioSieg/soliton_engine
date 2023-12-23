// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../api_prelude.hpp"

LUA_INTEROP_API auto __lu_panic(const char* msg) -> void {
    panic(msg ? msg : "unknown error");
}

LUA_INTEROP_API auto __lu_ffi_cookie() -> std::uint32_t {
    return 0xfefec0c0;
}
