// Copyright (c) 2022 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <random>

#include "core.hpp"

class subsystem : public no_copy, public no_move {
private:
    friend class kernel;
    static inline std::mt19937_64 prng;
    [[nodiscard]] static auto gen_id() noexcept -> std::uint64_t {
        std::uniform_int_distribution<std::uint64_t> dist{0, std::numeric_limits<std::uint64_t>::max()};
        return dist(prng);
    }
protected:
    explicit subsystem(std::string&& name) noexcept
        : name{std::move(name)}, id{gen_id()} {}
    virtual ~subsystem() = default;

    virtual auto on_prepare() -> void {} // called before the simulation loop is entered
    virtual auto on_resize() -> void {} // called on resize
    virtual auto on_pre_tick() -> bool { return true; } // called each frame
    virtual auto on_post_tick() -> void {} // called each frame in reverse

public:
    const std::string name;
    std::function<auto()->void> resize_hook {};
    const std::uint64_t id;
};
