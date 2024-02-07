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
        // This is the size of the temporary allocator that the physics system will use. This is used for temporary allocations.
        static constexpr std::uint32_t k_temp_allocator_size = 16ull << 20; // 16 MiB

        // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
        static constexpr std::uint32_t k_max_bodies = 0xffff;

        // This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
        static constexpr std::uint32_t k_num_mutexes = 0;

        // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
        // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
        // too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
        static constexpr std::uint32_t k_max_body_pairs = 0xffff;

        // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
        // number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
        static constexpr std::uint32_t k_max_contacts = 0x4000;

        // Max amount of threads that the physics system will use. This is set to 1/4 of the hardware concurrency.
        static inline const std::uint32_t k_num_threads = std::max(std::thread::hardware_concurrency() / 2, 2u);

        std::unique_ptr<JPH::TempAllocatorImpl> m_temp_allocator {};
        std::unique_ptr<JPH::JobSystemThreadPool> m_job_system {};
        std::unique_ptr<JPH::BroadPhaseLayerInterface> m_broad_phase {};
        std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> m_broad_phase_filter {};
        std::unique_ptr<JPH::ObjectLayerPairFilter> m_object_layer_pair_filter {};
        std::unique_ptr<JPH::ContactListener> m_contact_listener {};
        JPH::PhysicsSystem m_physics_system {};
    };
}
