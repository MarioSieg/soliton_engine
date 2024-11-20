// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "core.hpp"

namespace soliton {
    class scene;

    // Base class for all engine kernel subsystems
    class subsystem : public no_copy, public no_move {
    public:
        eastl::function<auto() -> void> resize_hook {};
        const eastl::string name;
        const std::uint32_t id;

        // time spent on this subsystem to construct instance of subsystem and initialize it (constructor call)
        [[nodiscard]] auto boot_time() const noexcept -> eastl::chrono::nanoseconds {
            return m_boot_time;
        }

        // time spent on this subsystem within on_prepare()
        [[nodiscard]] auto prepare_time() const noexcept -> eastl::chrono::nanoseconds {
            return m_prepare_time;
        }

        // time spent on this subsystem for total initialization (boot time + prepare time)
        [[nodiscard]] auto total_startup_time() const noexcept -> eastl::chrono::nanoseconds {
            return m_boot_time + m_prepare_time;
        }

        // time spent on this subsystem last time window was resized
        [[nodiscard]] auto prev_resize_time() const noexcept -> eastl::chrono::nanoseconds {
            return m_prev_resize_time;
        }

        // time spent on this subsystem last time start() was called for a new scene
        [[nodiscard]] auto prev_on_start_time() const noexcept -> eastl::chrono::nanoseconds {
            return m_prev_on_start_time;
        }

        // time spent on this subsystem last time pre_tick() was called
        [[nodiscard]] auto prev_pre_tick_time() const noexcept -> eastl::chrono::nanoseconds {
            return m_prev_pre_tick_time;
        }

        // time spent on this subsystem last time tick() was called
        [[nodiscard]] auto prev_tick_time() const noexcept -> eastl::chrono::nanoseconds {
            return m_prev_tick_time;
        }

        // time spent on this subsystem last time post_tick() was called
        [[nodiscard]] auto prev_post_tick_time() const noexcept -> eastl::chrono::nanoseconds {
            return m_prev_post_tick_time;
        }

        // time spent on this subsystem last time any of the tick functions was called
        [[nodiscard]] auto prev_tick_time_total() const noexcept -> eastl::chrono::nanoseconds {
            return m_prev_pre_tick_time + m_prev_tick_time + m_prev_post_tick_time;
        }

    protected:
        explicit subsystem(eastl::string&& name) noexcept
                : name{std::move(name)}, id{s_id_gen.fetch_add(1, std::memory_order_seq_cst)} {}
        virtual ~subsystem() = default;

        virtual auto on_prepare() -> void {} // called before the simulation loop is entered
        virtual auto on_resize() -> void {} // called on resize
        virtual auto on_start(scene& scene) -> void {} // called on scene start
        virtual auto on_pre_tick() -> bool { return true; } // called each frame
        virtual auto on_tick() -> void {} // called each frame
        virtual auto on_post_tick() -> void {} // called each frame in reverse

    private:
        friend class kernel;
        static inline constinit std::atomic_uint32_t s_id_gen = 1;
        eastl::chrono::nanoseconds m_boot_time {};
        eastl::chrono::nanoseconds m_prepare_time {};
        eastl::chrono::nanoseconds m_prev_resize_time {};
        eastl::chrono::nanoseconds m_prev_on_start_time {};
        eastl::chrono::nanoseconds m_prev_pre_tick_time {};
        eastl::chrono::nanoseconds m_prev_tick_time {};
        eastl::chrono::nanoseconds m_prev_post_tick_time {};
    };
}
