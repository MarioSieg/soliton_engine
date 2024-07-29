// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "scripting_subsystem.hpp"

#include "../scene/scene.hpp"

#include "libs/lfs/lfs.h"
// #include "libs/luv/luv.h" also defined a panic() function which is ambiguous with the one in core.hpp, so:
extern "C" int luaopen_luv (lua_State *L);

#include "convar.hpp"

#if USE_MIMALLOC
#include <mimalloc.h>
#endif

namespace lu::scripting {
    template <typename... Ts>
    static auto lua_log_info(const fmt::format_string<Ts...> fmt, Ts&&... args) -> void {
        SPDLOG_LOGGER_INFO(spdlog::get("app"), "[Lua]: {}", fmt::format(fmt, std::forward<Ts>(args)...));
    }

    template <typename... Ts>
    static auto lua_log_error(const fmt::format_string<Ts...> fmt, Ts&&... args) -> void {
        SPDLOG_LOGGER_ERROR(spdlog::get("app"), "[Lua]: {}", fmt::format(fmt, std::forward<Ts>(args)...));
    }

    scripting_subsystem::scripting_subsystem() : subsystem{"Scripting"} {
        log_info("Initializing scripting subsystem");
        log_info(LUAJIT_VERSION);
        lua_host_connect();
    }

    scripting_subsystem::~scripting_subsystem() {
        detail::disconnect_all_convars();
        lua_host_disconnect();
    }

    void scripting_subsystem::on_prepare() {
        passert(m_is_lua_host_online);
        if (const luabridge::LuaResult r = (*m_on_prepare)(); r.hasFailed()) [[unlikely]] {
            lua_log_error("{}: Error in {}: {}", k_boot_script, k_prepare_hook, r.errorMessage());
        }
    }

    HOTPROC void scripting_subsystem::on_tick() {
        passert(m_is_lua_host_online);
        if (const luabridge::LuaResult r = (*m_on_tick)(); r.hasFailed()) [[unlikely]] {
            lua_log_error("{}: Error in {}: {}", k_boot_script, k_tick_hook, r.errorMessage());
        }
    }

    auto scripting_subsystem::exec_file(const std::string& file) -> bool {
        // this file must be loaded without asset mgr, as asset mgr is not yet initialized and
        // assetmgr needs access to the engine config which is stored in a lua script
        std::string full_path {"/engine_assets/scripts/" + file};
        std::string lua_source_code {};
        assetmgr::with_primary_accessor_lock([&](assetmgr::asset_accessor &acc) {
            if (!acc.load_txt_file(full_path.c_str(), lua_source_code)) {
                lua_log_error("Failed to load script file '{}'", full_path);
            }
        });
        if (luaL_dostring(m_L, lua_source_code.c_str()) != LUA_OK) [[unlikely]] {
            lua_log_error("script error in {}: {}", full_path, lua_tostring(m_L, -1));
            lua_pop(m_L, 1);
            return false;
        }
        return true;
    }

    auto scripting_subsystem::lua_host_connect() -> void {
        log_info("Connecting to Lua host");

        if (m_is_lua_host_online) [[unlikely]] {
            log_warn("Lua host is already online");
            return;
        }

        // init lua
        passert(m_L == nullptr);
#if USE_MIMALLOC
        if constexpr (use_mimalloc) {
            /*
             * LuaJIT requires that allocated memory is in the first 47 bits of address space.
             * System malloc/mimalloc has no such guarantee of this, and hence can't (in general) be used.
             *
             * mimalloc most certainly doesn't support address-range restrictions.
             * If you really want to link with mimalloc, use the GC64 mode in the v2.1 branch, which doesn't have this limitation.
             * And do some performance testing, because it's unlikely it'll be better/faster/smaller than the built-in allocator.
             *
             */
            m_L = lua_newstate(+[]([[maybe_unused]] void* ud, void* ptr, [[maybe_unused]] std::size_t osize, const std::size_t nsize) noexcept -> void* {
                return mi_realloc(ptr, nsize);
            }, nullptr);
        } else
#endif
        {
            m_L = luaL_newstate();
        }
        passert(m_L != nullptr);

        // open base libraries
        luaL_openlibs(m_L);

        // open LFS
        passert(luaopen_lfs(m_L) == 1);

        // open LUV
        passert(luaopen_luv(m_L) == 1);
        lua_getglobal(m_L, "package");
        lua_getfield(m_L, -1, "loaded");
        lua_remove(m_L, -2);
        luaopen_luv(m_L);
        lua_setfield(m_L, -2, "luv");
        lua_pop(m_L, 1);

        luabridge::register_main_thread(m_L);

        static constexpr auto print_proxy = [](lua_State* l) -> int {
            for (int i = 1; i <= lua_gettop(l); ++i) {
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
        m_on_prepare = luabridge::getGlobal(m_L, k_prepare_hook);
        passert(m_on_prepare && m_on_prepare->isFunction());
        m_on_tick = luabridge::getGlobal(m_L, k_tick_hook);
        passert(m_on_tick && m_on_tick->isFunction());

        // init config table
        m_config_table = luabridge::getGlobal(m_L, k_engine_config_tab);
        passert(m_config_table && m_config_table->isTable());

        m_is_lua_host_online = true;
        log_info("Lua host connected");
    }

    auto scripting_subsystem::lua_host_disconnect() -> void {
        log_info("Shutting down Lua host");
        if (!m_is_lua_host_online) [[unlikely]] {
            log_warn("Lua host is already offline");
            return;
        }
        m_config_table.reset();
        m_on_tick.reset();
        m_on_prepare.reset();
        spdlog::get("app")->flush();
        lua_close(m_L);
        m_L = nullptr;
        m_is_lua_host_online = false;
        log_info("Lua host offline");
    }
}
