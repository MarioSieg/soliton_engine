// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include  "scripting_subsystem.hpp"

#include <stack>
#include <sstream>

namespace scripting {
    template <typename T>
    concept is_con_var_type = requires {
        std::is_integral_v<T> ||
        std::is_floating_point_v<T> ||
        std::is_same_v<T, std::string>;
    };

    struct convar_flags {
        enum $ {
            none = 0,
            read_only = 1 << 0,
        };
    };

    inline constinit std::uint32_t s_convar_i = 0;

    template <typename T> requires is_con_var_type<T>
    class convar final {
    public:
        convar(
            std::string&& name,
            std::optional<std::variant<T, std::function<auto() -> T>>>&& fallback,
            std::underlying_type_t<convar_flags::$> flags
        ) : m_name{std::move(name)}, m_fallback{std::move(fallback)}, m_flags{flags} {}

        [[nodiscard]] auto operator ()() -> T {
            register_var();
            if (!m_ref) [[unlikely]] {
                log_error("Failed to get convar '{}' as the engine config key value is not valid", full_name());
                return fallback<false>();
            }
            ++m_gets;
            return m_ref->cast<T>().valueOr(fallback<false>());
        }

        [[nodiscard]] auto full_name() const -> std::string_view { return m_name; }

    private:
        auto register_var() {
            if (m_ref) return;
            log_info("Registering convar #{} [{} : {}] | Flags: {:#x}, Fallback: {}", ++s_convar_i, full_name(), type_name(), m_flags, fallback<true>());
            passert(!m_name.empty() && "Convar must have a name");
            std::stack<luabridge::LuaRef> refs {};
            const auto* const root = scripting_subsystem::cfg();
            if (!root || !root->isTable()) [[unlikely]] {
                log_error("Failed to register convar '{}' as the engine config root table is not initialized yet", full_name());
                return;
            }
            refs.emplace(*root);
            std::stringstream ss {m_name};
            for (std::string k {}; std::getline(ss, k, '.');) {
                if (refs.top().isTable()) {
                    refs.emplace(refs.top()[k]);
                } else {
                    log_error("Failed to register convar '{}' as the engine config root table is not a table", full_name());
                    return;
                }
            }
            auto key = refs.top();
            if (!key.isValid()) [[unlikely]] {
                log_error("Failed to register convar '{}' as the engine config key value is not valid", full_name());
                return;
            }
            if constexpr (!std::is_same_v<T, bool> && (std::is_integral_v<T> || std::is_floating_point_v<T>)) {
                if (!key.isNumber()) {
                    log_error("Failed to register convar '{}' as the engine config key value is not of type 'number': '{}'", full_name(), key.tostring());
                    return;
                }
            } else if constexpr (std::is_same_v<T, bool>) {
                if (!key.isBool()) {
                    log_error("Failed to register convar '{}' as the engine config key value is not of type 'boolean': '{}'", full_name(), key.tostring());
                    return;
                }
            } else if constexpr (std::is_same_v<T, std::string>) {
                if (!key.isString()) {
                    log_error("Failed to register convar '{}' as the engine config key value is not of type 'string': '{}'", full_name(), key.tostring());
                    return;
                }
            }
            m_ref = key;
        }

        template<const bool None = false>
        [[nodiscard]] auto fallback() const -> T {
            if constexpr (None) return T{};
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
        const std::optional<std::variant<T, std::function<auto()->T>>> m_fallback;
        const std::underlying_type_t<convar_flags::$> m_flags;
        std::optional<luabridge::LuaRef> m_ref {};
        std::uint32_t m_gets {}, m_sets {};
    };
}

using scripting::convar;
using scripting::convar_flags;
