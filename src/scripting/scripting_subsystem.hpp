// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/subsystem.hpp"

#include <lua.hpp>

#include <LuaBridge/LuaBridge.h>

namespace lu::scripting {
    class scripting_subsystem final : public subsystem {
    public:
        static constexpr bool use_mimalloc = true; // use mimalloc over LuaJIT's allocator

        scripting_subsystem();
        ~scripting_subsystem() override;

        auto on_prepare() -> void override;
        HOTPROC auto on_tick() -> void override;
        [[nodiscard]] auto is_lua_host_online() const noexcept -> bool { return m_is_lua_host_online; }

        [[nodiscard]] static auto get_lua_state() noexcept -> lua_State* { return m_L; }
        // get global engine config table which is filled in from Lua (scripts/config/engine.lua)
        [[nodiscard]] static auto cfg() noexcept -> const luabridge::LuaRef* {
            return m_config_table ? &*m_config_table : nullptr;
        }

        static auto exec_file(const eastl::string& file) -> bool;

    private:
        bool m_is_lua_host_online = false;
        auto lua_host_connect() -> void;
        auto lua_host_disconnect() -> void;
        static constexpr const char* k_boot_script = "system/__boot__.lua";
        static constexpr const char* k_prepare_hook = "__on_prepare__";
        static constexpr const char* k_tick_hook = "__on_tick__";
        static constexpr const char* k_engine_config_tab = "engine_cfg";
        static inline constinit lua_State* m_L = nullptr;
        static inline eastl::optional<luabridge::LuaRef> m_config_table {};
        eastl::optional<luabridge::LuaRef> m_on_prepare {}; // Reference to __boot__.lua's on_prepare function
        eastl::optional<luabridge::LuaRef> m_on_tick {}; // Reference to __boot__.lua's on_tick function
    };
}
