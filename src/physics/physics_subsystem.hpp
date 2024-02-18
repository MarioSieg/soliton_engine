// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <optional>

#include "../core/subsystem.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace physics {
    class physics_subsystem final : public subsystem {
    public:
        physics_subsystem();
        ~physics_subsystem() override;

        auto on_start(scene& scene) -> void override;
        HOTPROC auto on_post_tick() -> void override;

    private:
        auto create_melons(scene& scene) -> void;

        std::unique_ptr<JPH::TempAllocatorImpl> m_temp_allocator {};
        std::unique_ptr<JPH::JobSystemThreadPool> m_job_system {};
        std::unique_ptr<JPH::BroadPhaseLayerInterface> m_broad_phase {};
        std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> m_broad_phase_filter {};
        std::unique_ptr<JPH::ObjectLayerPairFilter> m_object_layer_pair_filter {};
        std::unique_ptr<JPH::ContactListener> m_contact_listener {};
        JPH::PhysicsSystem m_physics_system {};
    };
}
