// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <random>

#include "core.hpp"

namespace lu {
    class scene;
    class subsystem : public no_copy, public no_move {
    private:
        friend class kernel;
        static inline constinit std::atomic_uint64_t s_id_gen = 1;

    protected:
        explicit subsystem(std::string&& name) noexcept
                : name{std::move(name)}, id{s_id_gen.fetch_add(1, std::memory_order_seq_cst)} {}
        virtual ~subsystem() = default;

        virtual auto on_prepare() -> void {} // called before the simulation loop is entered
        virtual auto on_resize() -> void {} // called on resize
        virtual auto on_start(scene& scene) -> void {} // called on scene start
        virtual auto on_pre_tick() -> bool { return true; } // called each frame
        virtual auto on_tick() -> void {} // called each frame
        virtual auto on_post_tick() -> void {} // called each frame in reverse

    public:
        const std::string name;
        std::function<auto() -> void> resize_hook {};
        const std::uint64_t id;
    };
}
