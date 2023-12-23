// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/subsystem.hpp"

#include <lua.hpp>

namespace scripting {
    class scripting_subsystem final : public subsystem {
    public:
        scripting_subsystem();
        ~scripting_subsystem() override;

        [[nodiscard]] static auto get_lua_state() noexcept -> lua_State* { return m_L; }

        static auto exec_file(const char* file) -> bool;

    private:
        static inline constinit lua_State* m_L = nullptr;
    };
}
