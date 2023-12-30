// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/core.hpp"

#if PLATFORM_WINDOWS
#define LUA_INTEROP_API extern "C" __cdecl __declspec(dllexport)
#else
#define LUA_INTEROP_API extern "C" __attribute__((visibility("default")))
#endif
