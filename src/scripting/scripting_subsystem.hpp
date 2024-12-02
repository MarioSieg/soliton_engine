// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/subsystem.hpp"

#include <lua.hpp>

#include <LuaBridge/LuaBridge.h>

namespace soliton::scripting {
    class scripting_subsystem final : public subsystem {
    public:
        scripting_subsystem();
        ~scripting_subsystem() override;

        auto on_start(scene&) -> void override;
        HOTPROC auto on_tick() -> void override;
        [[nodiscard]] auto is_lua_host_online() const noexcept -> bool { return m_is_lua_host_online; }

        [[nodiscard]] static auto get_lua_state() noexcept -> lua_State* { return m_L; }

        static auto exec_file(const eastl::string& file) -> bool;

    private:
        bool m_is_lua_host_online = false;
        auto lua_host_connect() -> void;
        auto lua_host_disconnect() -> void;
        static constexpr const char* k_boot_script = "system/__boot__.lua";
        static constexpr const char* k_start_hook = "__on_start__";
        static constexpr const char* k_tick_hook = "__on_tick__";
        static constexpr const char* k_engine_config_tab = "engine_cfg";
        static inline constinit lua_State* m_L = nullptr;
        eastl::optional<luabridge::LuaRef> m_on_start {}; // Reference to __boot__.lua's on_start function
        eastl::optional<luabridge::LuaRef> m_on_tick {}; // Reference to __boot__.lua's on_tick function
    };
}
