// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/subsystem.hpp"

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

namespace scripting {
    class scripting_subsystem final : public subsystem {
    public:
        scripting_subsystem();
        ~scripting_subsystem() override;

        auto on_prepare() -> void override;
        auto on_post_tick() -> void override;

        [[nodiscard]] static auto get_lua_state() noexcept -> lua_State* { return m_L; }

        static auto exec_file(const char* file) -> bool;

    private:
        static inline constinit lua_State* m_L = nullptr;
        std::optional<luabridge::LuaRef> m_on_prepare = {}; // Reference to __boot__.lua's on_prepare function
        std::optional<luabridge::LuaRef> m_on_tick = {}; // Reference to __boot__.lua's on_tick function
    };
}