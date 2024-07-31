// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include  "scripting_subsystem.hpp"

#include <stack>
#include <sstream>

namespace lu::scripting {
    // TODO: This file is a mess, needs to be cleaned up and refactored (use template specializations for each type)
    // I wrote this in hurry when I had no sleep for 24 hours, so it's a bit of a mess

    template <typename T>
    concept is_con_var_type = requires {
        std::is_default_constructible_v<T>;
        std::is_integral_v<T> ||
        std::is_floating_point_v<T> ||
        std::is_same_v<T, eastl::string>;
    };

    namespace detail {
        inline constinit std::uint32_t s_convar_i = 0;
        inline const auto tid = std::this_thread::get_id();
        inline eastl::vector<eastl::optional<luabridge::LuaRef>*> s_convars {};

        inline auto disconnect_all_convars() -> void {
            log_info("Disconnecting {} CONVARs", s_convars.size());
            for (auto* const ref : s_convars) {
                if (ref) ref->reset();
            }
            s_convars.clear();
        }
    }

    #define lu_convar_check(expr, msg) if (!(expr)) { log_error("Failed to register system variable {}: {}", #expr, msg); return false; }

    template <typename T> requires is_con_var_type<T>
    class system_variable final {
    public:
        constexpr system_variable(
            const char* const name,
            eastl::variant<eastl::monostate, T, eastl::function<auto() -> T>>&& fallback,
            const bool is_read_only = false
        ) : m_name{name}, m_fallback{std::move(fallback)}, m_is_read_only{is_read_only} {}

        [[nodiscard]] auto operator()() const -> T {
            if (!register_hook())
                return get_fallback();
            T val {};
            if constexpr (std::is_same_v<T, eastl::string>) {
                val = m_lua_ref->cast<std::string>().valueOr(get_fallback().c_str()).c_str();
            } else {
                val = m_lua_ref->cast<T>().valueOr(get_fallback());
            }
            ++m_num_queries;
            return val;
        }
        auto operator()(T&& val) -> void {
            panic("NYI");
        }

    private:
        [[nodiscard]] auto register_hook() const -> bool {
            using ref = luabridge::LuaRef;
            if (m_lua_ref) return true;
            lu_convar_check(detail::tid == std::this_thread::get_id(), "Must be registered on the main thread");
            lu_convar_check(std::strlen(m_name) > 0, "Empty name");
            log_info("Registering system variable '{}', type: {}, readonly: {}", m_name, get_lua_type_name(), m_is_read_only);
            const ref* const root = scripting_subsystem::cfg();
            lu_convar_check(root != nullptr, "Root config table not initialized");
            lu_convar_check(root->isTable(), "Root config table is not a table");
            eastl::fixed_vector<ref, 32> ref_stack {};
            ref_stack.emplace_back(*root);
            std::stringstream ss {m_name};
            constexpr char token_sep = '.';
            for (std::string token {}; std::getline(ss, token, token_sep); ) {
                lu_convar_check(ref_stack.back().isTable(), "Root config table is not a table");
                ref_stack.emplace_back(ref_stack.back()[token]);
            }
            ref key = ref_stack.back();
            lu_convar_check(key.isValid(), "Key value is not valid");
            lu_convar_check(!key.isNil(), "Key value is nil");
            if constexpr (std::is_same_v<T, eastl::string>) {
                lu_convar_check(key.isString(), "Type mismatch, expected 'string'");
            } else if constexpr (std::is_same_v<T, bool>) {
                lu_convar_check(key.isString(), "Type mismatch, expected 'boolean'");
            } else if constexpr(std::is_integral_v<T> || std::is_floating_point_v<T>) {
                lu_convar_check(key.isNumber(), "Type mismatch, expected 'number'");
            }
            m_lua_ref = std::move(key);
            detail::s_convars.emplace_back(&m_lua_ref);
            return true;
        }

        [[nodiscard]] constexpr auto get_lua_type_name() const noexcept -> eastl::string_view {
            if constexpr (std::is_same_v<T, eastl::string>) {
                return "string";
            } else if constexpr (std::is_same_v<T, bool>) {
                return "bool";
            } else if constexpr(std::is_integral_v<T> || std::is_floating_point_v<T>) {
                return "number";
            } else {
                return "unknown";
            }
        }

        [[nodiscard]] auto get_fallback() const -> T {
            if (eastl::holds_alternative<T>(m_fallback))
                return eastl::get<T>(m_fallback);
            else if (eastl::holds_alternative<eastl::function<auto() -> T>>(m_fallback))
                return eastl::invoke(eastl::get<eastl::function<auto() -> T>>(m_fallback));
            else return T {};
        }

        const char* const m_name;
        const eastl::variant<eastl::monostate, T, eastl::function<auto() -> T>> m_fallback;
        mutable eastl::optional<luabridge::LuaRef> m_lua_ref {};
        mutable std::uint32_t m_num_queries {};
        bool m_is_read_only = false;
    };

    #undef lu_convar_check

    template class system_variable<bool>;
    template class system_variable<float>;
    template class system_variable<double>;
    template class system_variable<std::byte>;
    template class system_variable<std::int8_t>;
    template class system_variable<std::uint8_t>;
    template class system_variable<std::int16_t>;
    template class system_variable<std::uint16_t>;
    template class system_variable<std::int32_t>;
    template class system_variable<std::uint32_t>;
    template class system_variable<std::int64_t>;
    template class system_variable<std::uint64_t>;
    template class system_variable<eastl::string>;
}

using lu::scripting::system_variable;
