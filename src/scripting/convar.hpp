// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include  "scripting_subsystem.hpp"

#include <stack>
#include <sstream>

namespace scripting {
    template <typename T>
    concept is_con_var_type = requires {
        std::is_default_constructible_v<T>;
        std::is_integral_v<T> ||
        std::is_floating_point_v<T> ||
        std::is_same_v<T, std::string>;
    };

    struct convar_flags {
        enum $ {
            none = 0,
            read_only = 1 << 0,
            no_clamp = 1 << 1,
        };
    };

    namespace detail {
        using convar_ref = std::pair<std::optional<luabridge::LuaRef>*, bool*>;
        inline constinit std::uint32_t s_convar_i = 0;
        inline const auto tid = std::this_thread::get_id();
        inline std::vector<convar_ref> s_convars {};

        inline auto disconnect_all_convars() -> void {
            log_info("Disconnecting {} CONVARs", s_convars.size());
            for (const auto& [ref, is_locked] : s_convars) {
                if (is_locked) *is_locked = true;
                if (ref) ref->reset();
            }
            s_convars.clear();
        }
    }

    template <typename T> requires is_con_var_type<T>
    class convar final {
    public:
        convar(
            std::string&& name,
            std::optional<std::variant<T, std::function<auto() -> T>>>&& fallback,
            std::underlying_type_t<convar_flags::$> flags,
            const T min = {},
            const T max = {}
        ) noexcept : m_name{std::move(name)}, m_fallback{std::move(fallback)}, m_flags{flags}, m_min{min}, m_max{max} {}

        [[nodiscard]] auto operator ()() -> T {
            if (m_is_locked) [[unlikely]] {
                log_error("Failed to get CONVAR '{}': Variable write is locked", full_name());
                return fallback();
            }
            register_var();
            if (!m_ref) [[unlikely]] {
                log_error("Failed to get CONVAR '{}': Engine config key not valid", full_name());
                return fallback();
            }
            ++m_gets;
            const T rv = m_ref->cast<T>().valueOr(fallback());
            if constexpr (std::is_same_v<T, bool> && std::is_integral_v<T> || std::is_floating_point_v<T>) {
                if (~(m_flags & convar_flags::no_clamp)) {
                    return std::clamp(rv, m_min, m_max);
                }
            }
            return rv;
        }

        [[nodiscard]] auto full_name() const -> std::string_view { return m_name; }

    private:
        auto register_var() {
            if (m_ref) return;
            passert(detail::tid == std::this_thread::get_id() && "Convars must be registered on the main thread");
            std::string s_fallback {"None"};
            if constexpr (std::is_same_v<T, std::string> | std::is_same_v<T, bool>) {
                if (m_fallback) {
                    if constexpr (std::is_same_v<T, std::string>) s_fallback = fallback();
                    else s_fallback = fallback() ? "true" : "false";
                }
               log_info("Registering CONVAR #{} [{} : {}] | Flags: {:#x}, Fallback: {}", ++detail::s_convar_i, full_name(), type_name(), m_flags, s_fallback);
            } else {
                if (m_fallback) {
                    s_fallback = std::to_string(fallback());
                }
                bool has_range = false;
                if constexpr(std::is_floating_point_v<T>) {
                    has_range = std::abs(m_min) > std::numeric_limits<T>::epsilon() || std::abs(m_max) > std::numeric_limits<T>::epsilon();
                } else {
                    has_range = m_min != std::numeric_limits<T>::min() || m_max != std::numeric_limits<T>::max();
                }
                std::string range {};
                if (has_range) {
                    range = fmt::format(", Range: [{}, {}]", m_min, m_max);
                }
                log_info("Registering CONVAR #{} [{} : {}] | Flags: {:#x}, Fallback: {}{}", ++detail::s_convar_i, full_name(), type_name(), m_flags, s_fallback, range);
            }
            if constexpr (std::is_same_v<T, bool> && std::is_integral_v<T> || std::is_floating_point_v<T>) {
                if (m_min > m_max) [[unlikely]] {
                    panic("Failed to create CONVAR '{}': The min value is greater than the max value", full_name());
                    return;
                }
                if constexpr (std::is_floating_point_v<T>) {
                    if (std::abs(m_min) < std::numeric_limits<T>::epsilon())
                        m_min = std::numeric_limits<T>::min();
                    if (std::abs(m_max) < std::numeric_limits<T>::epsilon())
                        m_max = std::numeric_limits<T>::max();
                } else {
                    if (m_min == 0) m_min = std::numeric_limits<T>::min();
                    if (m_max == 0) m_max = std::numeric_limits<T>::max();
                }
            } else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, bool>) {
                if (!m_min.empty() || !m_max.empty()) [[unlikely]] {
                    panic("Failed to create CONVAR '{}', string/bool variables cannot have min/max clamps", full_name());
                    return;
                }
            }
            if (m_name.empty()) [[unlikely]] {
                panic("Failed to create CONVAR: Empty name");
                return;
            }
            std::stack<luabridge::LuaRef> refs {};
            const auto* const root = scripting_subsystem::cfg();
            if (!root || !root->isTable()) [[unlikely]] {
                log_error("Failed to register CONVAR '{}': Engine config root table is not initialized yet", full_name());
                return;
            }
            refs.emplace(*root);
            std::stringstream ss {m_name};
            for (std::string k {}; std::getline(ss, k, '.');) {
                if (refs.top().isTable()) {
                    refs.emplace(refs.top()[k]);
                } else {
                    log_error("Failed to register CONVAR '{}': Engine config root table is not a table", full_name());
                    return;
                }
            }
            auto key = refs.top();
            if (!key.isValid()) [[unlikely]] {
                log_error("Failed to register CONVAR '{}': Engine config key value is not valid", full_name());
                return;
            }
            if constexpr (!std::is_same_v<T, bool> && (std::is_integral_v<T> || std::is_floating_point_v<T>)) {
                if (!key.isNumber()) {
                    log_error("Failed to register CONVAR '{}': Engine config key value is not of type 'number': '{}'", full_name(), key.tostring());
                    return;
                }
            } else if constexpr (std::is_same_v<T, bool>) {
                if (!key.isBool()) {
                    log_error("Failed to register CONVAR '{}': Engine config key value is not of type 'boolean': '{}'", full_name(), key.tostring());
                    return;
                }
            } else if constexpr (std::is_same_v<T, std::string>) {
                if (!key.isString()) {
                    log_error("Failed to register CONVAR '{}': Engine config key value is not of type 'string': '{}'", full_name(), key.tostring());
                    return;
                }
            }
            m_ref = key;
            detail::s_convars.emplace_back(std::make_pair(&m_ref, &m_is_locked));
        }

        [[nodiscard]] auto fallback() const -> T {
            if (std::holds_alternative<T>(*m_fallback)) {
                return std::get<T>(*m_fallback);
            } else {
                return std::get<std::function<auto()->T>>(*m_fallback)();
            }
        }

        static constexpr auto type_name() noexcept -> std::string_view {
            if constexpr (!std::is_same_v<T, bool> && (std::is_integral_v<T> || std::is_floating_point_v<T>)) return "number";
            else if constexpr (std::is_same_v<T, bool>) return "boolean";
            else if constexpr (std::is_same_v<T, std::string>) return "string";
            else return "unknown";
        }

        const std::string m_name;
        const std::optional<std::variant<T, std::function<auto() -> T>>> m_fallback;
        const std::underlying_type_t<convar_flags::$> m_flags;
        T m_min {};
        T m_max {};
        std::optional<luabridge::LuaRef> m_ref {};
        std::uint32_t m_gets {}, m_sets {};
        bool m_is_locked {};
    };
}

using scripting::convar;
using scripting::convar_flags;
