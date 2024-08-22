// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <optional>

#include "../core/subsystem.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "debug_renderer.hpp"
#include "../scene/components.hpp"

namespace lu::physics {
    class physics_subsystem final : public subsystem {
    public:
        physics_subsystem();
        ~physics_subsystem() override;

        auto on_start(scene& scene) -> void override;
        HOTPROC auto on_post_tick() -> void override;

        [[nodiscard]] static auto get_physics_system() noexcept -> JPH::PhysicsSystem& {
            return m_physics_system;
        }

        [[nodiscard]] static auto get_debug_renderer() noexcept -> debug_renderer& {
            return *m_debug_renderer;
        }

    private:
        static auto create_static_body(JPH::BodyCreationSettings& ci, const com::transform& transform, const com::mesh_renderer& renderer) -> void;

        auto post_sync() const -> void;
        eastl::unique_ptr<JPH::TempAllocatorImpl> m_temp_allocator {};
        eastl::unique_ptr<JPH::JobSystemThreadPool> m_job_system {};
        eastl::unique_ptr<JPH::BroadPhaseLayerInterface> m_broad_phase {};
        eastl::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> m_broad_phase_filter {};
        eastl::unique_ptr<JPH::ObjectLayerPairFilter> m_object_layer_pair_filter {};
        eastl::unique_ptr<JPH::ContactListener> m_contact_listener {};
        static inline JPH::PhysicsSystem m_physics_system {};
        static inline eastl::unique_ptr<debug_renderer> m_debug_renderer {};
        eastl::vector<JPH::BodyID> m_static_bodies {};
    };
}
