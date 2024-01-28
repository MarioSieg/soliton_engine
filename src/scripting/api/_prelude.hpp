// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../../core/core.hpp"
#include "../../scene/scene.hpp"
#include "../../math/DirectXMath.h"

#if PLATFORM_WINDOWS
#define LUA_INTEROP_API extern "C" __cdecl __declspec(dllexport)
#else
#define LUA_INTEROP_API extern "C" __attribute__((visibility("default")))
#endif


// Vector2 for LUA interop
// Only a proxy type holding the data and allowing implicit conversions to other vector types.
// Note that each conversion will result in a cast or vector cast.
struct lua_vec2 {
    double x;
    double y;

    constexpr lua_vec2() noexcept = default;
    constexpr lua_vec2(const double x, const double y) noexcept : x{x}, y{y} {}
    constexpr lua_vec2(const XMFLOAT2& vec) noexcept
        : x{static_cast<float>(vec.x)}, y{static_cast<float>(vec.y)} {}

    [[nodiscard]] constexpr operator XMFLOAT2 () const noexcept {
        return {
            static_cast<float>(x),
            static_cast<float>(y)
        };
    }
    [[nodiscard]] operator XMVECTOR() const noexcept {
        const XMFLOAT2 tmp = *this;
        return XMLoadFloat2(&tmp);
    }
};
static_assert(sizeof(lua_vec2) == sizeof(double) * 2 && std::is_standard_layout_v<lua_vec2>);

// Vector3 for LUA interop
// Only a proxy type holding the data and allowing implicit conversions to other vector types.
// Note that each conversion will result in a cast or vector cast.
struct lua_vec3 {
    double x;
    double y;
    double z;

    constexpr lua_vec3() noexcept = default;
    constexpr lua_vec3(const double x, const double y, const double z) noexcept
        : x{x}, y{y}, z{z} {}
    constexpr lua_vec3(const XMFLOAT3& vec) noexcept
        : x{static_cast<float>(vec.x)}
        , y{static_cast<float>(vec.y)}
        , z{static_cast<float>(vec.z)} {}
    constexpr lua_vec3(const XMFLOAT4& vec) noexcept
       : x{static_cast<float>(vec.x)}
    , y{static_cast<float>(vec.y)}
    , z{static_cast<float>(vec.z)} {}

    [[nodiscard]] constexpr operator XMFLOAT2 () const noexcept {
        return {
            static_cast<float>(x),
            static_cast<float>(y)
        };
    }
    [[nodiscard]] constexpr operator XMFLOAT3 () const noexcept {
        return {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z)
        };
    }
    [[nodiscard]] constexpr operator XMFLOAT4 () const noexcept {
        return {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z),
            0.0f
        };
    }
    [[nodiscard]] operator XMVECTOR() const noexcept {
        const XMFLOAT3 tmp = *this;
        return XMLoadFloat3(&tmp);
    }
};
static_assert(sizeof(lua_vec3) == sizeof(double) * 3 && std::is_standard_layout_v<lua_vec3>);

// Vector4 for LUA interop
// Only a proxy type holding the data and allowing implicit conversions to other vector types.
// Note that each conversion will result in a cast or vector cast.
struct lua_vec4 {
    double x;
    double y;
    double z;
    double w;

    constexpr lua_vec4() noexcept = default;
    constexpr lua_vec4(const double x, const double y, const double z, const double w) noexcept
        : x{x}, y{y}, z{z}, w{w} {}
    constexpr lua_vec4(const XMFLOAT4& vec) noexcept
    : x{static_cast<float>(vec.x)}
    , y{static_cast<float>(vec.y)}
    , z{static_cast<float>(vec.z)}
    , w{static_cast<float>(vec.w)} {}

    [[nodiscard]] constexpr operator XMFLOAT4() const noexcept {
        return {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z),
            static_cast<float>(w)
        };
    }
    [[nodiscard]] operator XMVECTOR() const noexcept{
        const XMFLOAT4 tmp = *this;
        return XMLoadFloat4(&tmp);
    }
};
static_assert(sizeof(lua_vec4) == sizeof(double) * 4 && std::is_standard_layout_v<lua_vec4>);
