// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/core.hpp"
#include "../math/DirectXMath.h"

#include <bx/math.h>

#if PLATFORM_WINDOWS
#define LUA_INTEROP_API extern "C" __cdecl __declspec(dllexport)
#else
#define LUA_INTEROP_API extern "C" __attribute__((visibility("default")))
#endif

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
