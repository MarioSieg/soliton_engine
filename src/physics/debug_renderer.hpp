// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/subsystem.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Renderer/DebugRenderer.h>

namespace lu::physics {
    class debug_renderer final : public JPH::DebugRenderer {
    public:
        static auto begin() -> void;
        static auto end() -> void;

    protected:
        virtual auto DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to, JPH::ColorArg color) -> void override;
        virtual auto DrawTriangle(JPH::RVec3Arg a, JPH::RVec3Arg b, JPH::RVec3Arg c, JPH::ColorArg color, ECastShadow shadow) -> void override;
        virtual auto DrawGeometry(
            JPH::RMat44Arg mtx,
            const JPH::AABox& aabb,
            float lod,
            JPH::ColorArg color,
            const GeometryRef& geom,
            ECullMode cull,
            ECastShadow shadow,
            EDrawMode mode
        ) -> void override;
        virtual auto DrawText3D(JPH::RVec3Arg inPosition, const JPH::string_view& inString, JPH::ColorArg inColor, float height) -> void override;
        virtual auto CreateTriangleBatch(const Triangle* tris, int count) -> Batch override;
        virtual auto CreateTriangleBatch(const Vertex* vertices, int vcount, const std::uint32_t* indices, int icount) -> Batch override;
    };
}
