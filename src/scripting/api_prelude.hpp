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

struct lua_vec2 {
    double x;
    double y;

    [[nodiscard]] auto to_vec2() const noexcept -> DirectX::XMFLOAT2 {
        return {
            static_cast<float>(x),
            static_cast<float>(y)
        };
    }
    [[nodiscard]] auto to_xmvec() const noexcept -> DirectX::XMVECTOR {
        const DirectX::XMFLOAT2 tmp = to_vec2();
        return DirectX::XMLoadFloat2(&tmp);
    }
};
static_assert(sizeof(lua_vec2) == sizeof(double) * 2 && std::is_standard_layout_v<lua_vec2>);

struct lua_vec3 {
    double x;
    double y;
    double z;

    [[nodiscard]] auto to_vec3() const noexcept -> DirectX::XMFLOAT3 {
        return {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z)
        };
    }
    [[nodiscard]] auto to_xmvec() const noexcept -> DirectX::XMVECTOR {
        const DirectX::XMFLOAT3 tmp = to_vec3();
        return DirectX::XMLoadFloat3(&tmp);
    }
    [[nodiscard]] auto to_bxvec() const noexcept -> bx::Vec3 {
        return {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z)
        };
    }
};
static_assert(sizeof(lua_vec3) == sizeof(double) * 3 && std::is_standard_layout_v<lua_vec3>);

struct lua_vec4 {
    double x;
    double y;
    double z;
    double w;

    [[nodiscard]] auto to_vec4() const noexcept -> DirectX::XMFLOAT4 {
        return {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z),
            static_cast<float>(w)
        };
    }
    [[nodiscard]] auto to_xmvec() const noexcept -> DirectX::XMVECTOR {
        const DirectX::XMFLOAT4 tmp = to_vec4();
        return DirectX::XMLoadFloat4(&tmp);
    }
};
static_assert(sizeof(lua_vec4) == sizeof(double) * 4 && std::is_standard_layout_v<lua_vec4>);
