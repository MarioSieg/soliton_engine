// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include  "scripting_subsystem.hpp"

namespace scripting {
    template <typename T>
    concept is_con_var_type = requires {
        std::is_integral_v<T> ||
        std::is_floating_point_v<T> ||
        std::is_same_v<T, bool> ||
        std::is_same_v<T, std::string>;
    };

    template <typename T> requires is_con_var_type<T>
    class convar final {
    public:
        explicit convar(const std::string_view package, const std::string_view key, T&& fallback_val)
            : m_package {package}, m_key{key}, m_fallback{std::forward(fallback_val)} {
            log_info("Registering CONVAR {}.{} [{}]", package, key, typeid(T).name());
        }

        [[nodiscard]] auto operator ()() const -> T {
            const luabridge::LuaRef* const root_tab = scripting_subsystem::cfg();
            if (!root_tab) [[unlikely]] {
                log_warn("Engine config variable {}.{} from Lua requested, but engine config it not initialized yet", m_package, m_key);
                return T{m_fallback};
            }
            const luabridge::LuaRef package = (*root_tab)[m_package];
            if (!package) [[unlikely]] {
                log_warn("Package for engine config variable {}.{} not found: {}", m_package, m_key, m_package);
                return T{m_fallback};
            }
            const luabridge::LuaRef result = package[m_key];
            if (!result) [[unlikely]] {
                log_warn("Engine config variable {}.{} not found");
                return T{m_fallback};
            }
            if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
                if (!result.isNumber()) [[unlikely]] {
                    log_warn("Engine config variable {}.{} must be of type 'number'");
                    return T{m_fallback};
                }
            } else if constexpr (std::is_same_v<T, bool>) {
                if (!result.isBool()) [[unlikely]] {
                    log_warn("Engine config variable {}.{} must be of type 'bool'");
                    return T{m_fallback};
                }
            } else if constexpr (std::is_same_v<T, std::string>) {
                if (!result.isNumber()) [[unlikely]] {
                    log_warn("Engine config variable {}.{} must be of type 'string'");
                    return T{m_fallback};
                }
            }
            return result.cast<T>().valueOr(m_fallback);
        }

    private:
        const std::string_view m_package;
        const std::string_view m_key;
        const T m_fallback;
    };
}
