// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "subsystem.hpp"

#include <lfs.c>

namespace scripting {
    template <typename... Ts>
    static auto lua_log_info(const fmt::format_string<Ts...> fmt, Ts&&... args) -> void {
        SPDLOG_LOGGER_INFO(spdlog::get("App"), "[Lua]: {}", fmt::format(fmt, std::forward<Ts>(args)...));
    }

    template <typename... Ts>
    static auto lua_log_error(const fmt::format_string<Ts...> fmt, Ts&&... args) -> void {
        SPDLOG_LOGGER_ERROR(spdlog::get("App"), "[Lua]: {}", fmt::format(fmt, std::forward<Ts>(args)...));
    }

    scripting_subsystem::scripting_subsystem() : subsystem{"Scripting"} {
        log_info("Initializing scripting subsystem");
        log_info(LUAJIT_VERSION);

        // init lua
        passert(m_L == nullptr);
        m_L = luaL_newstate();
        passert(m_L != nullptr);
        luaL_openlibs(m_L);
        passert(luaopen_lfs(m_L) == 1);

        static constexpr auto print_proxy = [](lua_State* l) -> int {
            for (int i  = 1; i <= lua_gettop(l); ++i) {
                switch (lua_type(l, i)) {
                    case LUA_TNIL: lua_log_info("nil"); break;
                    case LUA_TNUMBER: lua_log_info("{}", lua_tonumber(l, i)); break;
                    case LUA_TBOOLEAN: lua_log_info("{}", static_cast<bool>(lua_toboolean(l, i))); break;
                    case LUA_TSTRING: lua_log_info("{}", lua_tostring(l, i)); break;
                    default: ;
                }
            }
            return 0;
        };

        // print proxy
        static constexpr std::array<const luaL_Reg, 2> lib = {
            luaL_Reg { "print", +print_proxy },
            luaL_Reg { nullptr, nullptr } // terminator
        };
        lua_getglobal(m_L, "_G");
        luaL_setfuncs(m_L, lib.data(), 0);
        lua_pop(m_L, 1);

        // run boot script
        passert(exec_file(k_boot_script));

        // setup hooks
        m_on_prepare = luabridge::getGlobal(m_L, "__on_prepare__");
        m_on_tick = luabridge::getGlobal(m_L, "__on_tick__");
        passert(m_on_prepare && m_on_prepare->isFunction());
        passert(m_on_tick && m_on_tick->isFunction());
    }

    scripting_subsystem::~scripting_subsystem() {
        m_on_tick.reset();
        m_on_prepare.reset();
        spdlog::get("App")->flush();
        lua_close(m_L);
        m_L = nullptr;
    }

    void scripting_subsystem::on_prepare() {
        luabridge::LuaResult r = (*m_on_prepare)();
        if (r.hasFailed()) [[unlikely]] {
            lua_log_error("Error in __boot.lua in __on_prepare__: {}", r.errorMessage());
        }
    }

    void scripting_subsystem::on_tick() {
        luabridge::LuaResult r = (*m_on_tick)();
        if (r.hasFailed()) [[unlikely]] {
            lua_log_error("Error in __boot.lua in __on_tick__: {}", r.errorMessage());
        }
    }

    auto scripting_subsystem::exec_file(const char* file) -> bool {
        if (luaL_dofile(m_L, file) != LUA_OK) [[unlikely]] {
            lua_log_error("script error in {}: {}", file, lua_tostring(m_L, -1));
            lua_pop(m_L, 1);
            return false;
        }
        return true;
    }
}
