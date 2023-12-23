// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/core.hpp"

#if PLATFORM_WINDOWS
#define LUA_INTEROP_API __cdecl __declspec(dllexport) extern "C"
#else
#define LUA_INTEROP_API __attribute__((visibility("default"))) extern "C"
#endif
