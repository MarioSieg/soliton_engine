// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../../core/core.hpp"
#include "../../scene/scene.hpp"
#include "../../math/DirectXMath.h"

#include <../../rendercore/bx/include/bx/math.h>

#if PLATFORM_WINDOWS
#define LUA_INTEROP_API extern "C" __cdecl __declspec(dllexport)
#else
#define LUA_INTEROP_API extern "C" __attribute__((visibility("default")))
#endif

// We need to store the entity id which is 64-bits wide inside a double which has 52-bits mantissa.
// This means we can't store the full entity id inside a double.
// We can however store the lower 52-bits of the entity id inside a double.
// This is what we do here.
// LuaJIT can only handle 64-bit integers using the FFI which will allocate each integer on the heap, which sucks.

using lua_entity_id = double;

// std::isfinite() doesn't work with -ffast-math, so we need to do this ourselves:
[[nodiscard]] constexpr __attribute__((always_inline)) auto lua_entity_id_is_valid(const lua_entity_id x) noexcept -> bool {
    return ((std::bit_cast<id_t>(x) >> 52) & 0x7ff) != 0x7ff; // Check if exponent is not 0x7ff (infinity or NaN)
}

[[nodiscard]] inline __attribute__((always_inline)) auto lua_entity_id_to_entity_id(const lua_entity_id x) noexcept -> id_t {
    passert(lua_entity_id_is_valid(x));
    return std::bit_cast<id_t>(x);
}

[[nodiscard]] inline __attribute__((always_inline)) auto lua_entity_id_from_entity_id(const id_t x) noexcept -> lua_entity_id {
    const auto id = std::bit_cast<lua_entity_id>(x);
    passert(lua_entity_id_is_valid(id));
    return id;
}

[[nodiscard]] inline __attribute__((always_inline)) auto lua_entity_id_connect(const lua_entity_id x) noexcept -> entity {
    const auto& scene = scene::get_active();
    if (!scene) [[unlikely]] { return entity::null(); }
    return entity{scene->get_world(), lua_entity_id_to_entity_id(x)};
}


// Vector2 for LUA interop
// Only a proxy type holding the data and allowing implicit conversions to other vector types.
// Note that each conversion will result in a cast or vector cast.
struct lua_vec2 {
    double x;
    double y;

    constexpr __attribute__((always_inline)) lua_vec2() noexcept = default;
    constexpr __attribute__((always_inline)) lua_vec2(const double x, const double y) noexcept : x{x}, y{y} {}
    constexpr __attribute__((always_inline)) lua_vec2(const XMFLOAT2& vec) noexcept
        : x{static_cast<float>(vec.x)}, y{static_cast<float>(vec.y)} {}

    [[nodiscard]] constexpr __attribute__((always_inline)) operator XMFLOAT2 () const noexcept {
        return {
            static_cast<float>(x),
            static_cast<float>(y)
        };
    }
    [[nodiscard]] __attribute__((always_inline)) operator XMVECTOR() const noexcept {
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

    constexpr __attribute__((always_inline)) lua_vec3() noexcept = default;
    constexpr __attribute__((always_inline)) lua_vec3(const double x, const double y, const double z) noexcept
        : x{x}, y{y}, z{z} {}
    constexpr __attribute__((always_inline)) lua_vec3(const XMFLOAT3& vec) noexcept
        : x{static_cast<float>(vec.x)}
        , y{static_cast<float>(vec.y)}
        , z{static_cast<float>(vec.z)} {}
    constexpr __attribute__((always_inline)) lua_vec3(const bx::Vec3& vec) noexcept
        : x{static_cast<float>(vec.x)}
        , y{static_cast<float>(vec.y)}
        , z{static_cast<float>(vec.z)} {}

    [[nodiscard]] constexpr __attribute__((always_inline)) operator XMFLOAT2 () const noexcept {
        return {
            static_cast<float>(x),
            static_cast<float>(y)
        };
    }
    [[nodiscard]] constexpr __attribute__((always_inline)) operator XMFLOAT3 () const noexcept {
        return {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z)
        };
    }
    [[nodiscard]] constexpr __attribute__((always_inline)) operator bx::Vec3 () const noexcept {
        return {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z)
        };
    }
    [[nodiscard]] __attribute__((always_inline)) operator XMVECTOR() const noexcept {
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

    constexpr __attribute__((always_inline)) lua_vec4() noexcept = default;
    constexpr __attribute__((always_inline)) lua_vec4(const double x, const double y, const double z, const double w) noexcept
        : x{x}, y{y}, z{z}, w{w} {}
    constexpr __attribute__((always_inline)) lua_vec4(const XMFLOAT4& vec) noexcept
    : x{static_cast<float>(vec.x)}
    , y{static_cast<float>(vec.y)}
    , z{static_cast<float>(vec.z)}
    , w{static_cast<float>(vec.w)} {}

    [[nodiscard]] constexpr __attribute__((always_inline)) operator XMFLOAT4() const noexcept {
        return {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z),
            static_cast<float>(w)
        };
    }
    [[nodiscard]] __attribute__((always_inline)) operator XMVECTOR() const noexcept{
        const XMFLOAT4 tmp = *this;
        return XMLoadFloat4(&tmp);
    }
};
static_assert(sizeof(lua_vec4) == sizeof(double) * 4 && std::is_standard_layout_v<lua_vec4>);
