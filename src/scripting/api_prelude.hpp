// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/core.hpp"
#include "../scene/scene.hpp"
#include "../math/DirectXMath.h"

#include <bx/math.h>

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

// std::isfinite() doesn't work with -ffast-math, so we need to do this ourselves:
__attribute__((always_inline)) static constexpr auto is_double_valid_as_id(const double x) noexcept -> bool {
    return (std::bit_cast<std::uint64_t>(x) & 0x7ff0000000000000) >> 52 != 0x7ff; // Check if exponent is not 0x7ff (infinity or NaN)
}

// Static assert self test:
static_assert(is_double_valid_as_id(0.0) == true);
static_assert(is_double_valid_as_id(1.0) == true);
static_assert(is_double_valid_as_id(2.0) == true);
static_assert(is_double_valid_as_id(-3.0) == true);
static_assert(is_double_valid_as_id(std::numeric_limits<double>::quiet_NaN()) == false);
static_assert(is_double_valid_as_id(std::numeric_limits<double>::infinity()) == false);

[[maybe_unused]]
__attribute__((always_inline)) inline auto entity_id_to_double(const id_t id) noexcept -> double {
    const auto d = std::bit_cast<double>(id);
    passert(is_double_valid_as_id(d) && "Malformed entity id");
    return d;
}

[[maybe_unused]]
__attribute__((always_inline)) inline auto double_to_entity_id(const double d) noexcept -> id_t {
    passert(is_double_valid_as_id(d) && "Malformed entity id");
    return std::bit_cast<id_t>(d);
}

[[maybe_unused]] [[nodiscard]] __attribute__((always_inline)) inline auto entity_from_id(const double id) -> entity {
    const auto& scene = scene::get_active();
    if (scene == nullptr) [[unlikely]] return {};
    const id_t entity_id = double_to_entity_id(id);
    return entity{scene->get_world(), entity_id};
}

// Vector2 for LUA interop
// Only a proxy type holding the data and allowing implicit conversions to other vector types.
// Note that each conversion will result in a cast or vector cast.
struct lvec2 {
    double x = 0.0;
    double y = 0.0;

    constexpr __attribute__((always_inline)) lvec2() noexcept = default;
    constexpr __attribute__((always_inline)) lvec2(const double x, const double y) noexcept : x{x}, y{y} {}
    constexpr __attribute__((always_inline)) lvec2(const XMFLOAT2& vec) noexcept
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
static_assert(sizeof(lvec2) == sizeof(double) * 2 && std::is_standard_layout_v<lvec2>);

// Vector3 for LUA interop
// Only a proxy type holding the data and allowing implicit conversions to other vector types.
// Note that each conversion will result in a cast or vector cast.
struct lvec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    constexpr __attribute__((always_inline)) lvec3() noexcept = default;
    constexpr __attribute__((always_inline)) lvec3(const double x, const double y, const double z) noexcept
        : x{x}, y{y}, z{z} {}
    constexpr __attribute__((always_inline)) lvec3(const XMFLOAT3& vec) noexcept
        : x{static_cast<float>(vec.x)}
        , y{static_cast<float>(vec.y)}
        , z{static_cast<float>(vec.z)} {}
    constexpr __attribute__((always_inline)) lvec3(const bx::Vec3& vec) noexcept
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
static_assert(sizeof(lvec3) == sizeof(double) * 3 && std::is_standard_layout_v<lvec3>);

// Vector4 for LUA interop
// Only a proxy type holding the data and allowing implicit conversions to other vector types.
// Note that each conversion will result in a cast or vector cast.
struct lvec4 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double w = 0.0;

    constexpr __attribute__((always_inline)) lvec4() noexcept = default;
    constexpr __attribute__((always_inline)) lvec4(const double x, const double y, const double z, const double w) noexcept
        : x{x}, y{y}, z{z}, w{w} {}
    constexpr __attribute__((always_inline)) lvec4(const XMFLOAT4& vec) noexcept
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
static_assert(sizeof(lvec4) == sizeof(double) * 4 && std::is_standard_layout_v<lvec4>);
