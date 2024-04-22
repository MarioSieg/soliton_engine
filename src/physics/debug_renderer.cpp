// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "debug_renderer.hpp"
#include "../graphics/graphics_subsystem.hpp"

namespace physics {
    using graphics::graphics_subsystem;

#define dd (graphics_subsystem::s_instance->get_debug_draw())

    auto debug_renderer::begin() -> void {
        dd.begin_batch();
    }

    auto debug_renderer::end() -> void {
        dd.end_batch();
    }

    static constexpr DirectX::XMFLOAT3 k_physics_debug_color {1.0f, 0.0f, 0.0f};

    auto debug_renderer::DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to, const JPH::ColorArg color) -> void {
        dd.draw_line(
            std::bit_cast<DirectX::XMFLOAT3A>(from),
            std::bit_cast<DirectX::XMFLOAT3A>(to),
            k_physics_debug_color
        );
    }

    auto debug_renderer::DrawTriangle(
        JPH::RVec3Arg a,
        JPH::RVec3Arg b,
        JPH::RVec3Arg c,
        const JPH::ColorArg color,
        ECastShadow shadow
    ) -> void {
        dd.draw_line(
            std::bit_cast<DirectX::XMFLOAT3A>(a),
            std::bit_cast<DirectX::XMFLOAT3A>(b),
            k_physics_debug_color
        );
        dd.draw_line(
            std::bit_cast<DirectX::XMFLOAT3A>(b),
            std::bit_cast<DirectX::XMFLOAT3A>(c),
            k_physics_debug_color
        );
        dd.draw_line(
            std::bit_cast<DirectX::XMFLOAT3A>(c),
            std::bit_cast<DirectX::XMFLOAT3A>(a),
            k_physics_debug_color
        );
    }

    auto debug_renderer::DrawGeometry(
        JPH::RMat44Arg mtx,
        const JPH::AABox& aabb,
        float lod,
        JPH::ColorArg color,
        const GeometryRef& geom,
        ECullMode cull,
        ECastShadow shadow,
        EDrawMode mode
    ) -> void {
        static constinit int called = 0;
        if (called < 10) {
            log_warn("NYI");
            ++called;
        }
    }

    auto debug_renderer::DrawText3D(
        JPH::RVec3Arg inPosition,
        const JPH::string_view& inString,
        JPH::ColorArg inColor,
        float height
    ) -> void {
        static constinit int called = 0;
        if (called < 10) {
            log_warn("NYI");
            ++called;
        }
    }

    auto debug_renderer::CreateTriangleBatch(const Triangle* tris, int count) -> Batch {
        static constinit int called = 0;
        if (called < 10) {
            log_warn("NYI");
            ++called;
        }
        return {};
    }

    auto debug_renderer::CreateTriangleBatch(
        const Vertex* vertices,
        int vcount,
        const std::uint32_t* indices,
        int icount
    ) -> Batch {
        static constinit int called = 0;
        if (called < 10) {
            log_warn("NYI");
            ++called;
        }
        return {};
    }
}
