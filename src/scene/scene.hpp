// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "base.hpp"
#include "components.hpp"

class scene : public world, public no_copy, public no_move {
public:
    const std::uint32_t id;
    std::string name = {};
    virtual ~scene() = default;

    static auto new_active(std::string&& name = {}) -> void;
    [[nodiscard]] static auto get_active() noexcept -> const std::unique_ptr<scene>& { return m_active; }

    virtual auto on_tick() -> void;
    virtual auto on_start() -> void;

    [[nodiscard]] auto spawn(const char* name) const -> struct entity;

private:
    friend struct proxy;
    static inline constinit std::unique_ptr<scene> m_active {};
    scene();
};
