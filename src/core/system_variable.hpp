// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "core.hpp"

namespace soliton {
    template <typename T>
    concept sv_supported_type = requires {
        requires std::is_default_constructible_v<T>;
        requires std::is_same_v<bool, T> ||
            std::is_same_v<float, T> ||
            std::is_same_v<std::int64_t, T> ||
            std::is_same_v<eastl::string, T>;
    };

    template <typename T>
    struct sv_supported_type_name {};

    template <> struct sv_supported_type_name<bool> { static constexpr const char* value = "bool"; };
    template <> struct sv_supported_type_name<float> { static constexpr const char* value = "float"; };
    template <> struct sv_supported_type_name<std::int64_t> { static constexpr const char* value = "int64_t"; };
    template <> struct sv_supported_type_name<eastl::string> { static constexpr const char* value = "string"; };

    namespace detail {
        inline const auto tid = std::this_thread::get_id();
        using value = eastl::variant
        <
            eastl::monostate,
            bool,
            float,
            std::int64_t,
            eastl::string
        >;
        inline ankerl::unordered_dense::map<eastl::string, value> sv_registry {};
        inline std::mutex sv_mutex {}; // Well, we need to lock it. :( Better not use sv variables every frame from different threads.
        [[nodiscard]] extern auto save_system_variables() -> bool;
        [[nodiscard]] extern auto load_system_variables() -> bool;
    }

    // System variable class. Configurable at runtime via INI or command line.
    // Supported types: bool, float, int64_t, string.
    template <typename T> requires sv_supported_type<T>
    class system_variable final {
    public:
        constexpr system_variable(
            eastl::string&& name,
            eastl::variant<eastl::monostate, T, eastl::function<auto() -> T>>&& fallback
        ) : m_name{name}, m_fallback{std::move(fallback)} {}

        [[nodiscard]] auto operator()() const -> T {
            std::lock_guard lock {detail::sv_mutex};
            auto& reg = detail::sv_registry;
            if (reg.contains(m_name)) [[likely]] { // If the variable is already registered, return it.
                detail::value& val = reg[m_name];
                if (eastl::holds_alternative<T>(val)) [[likely]] {
                    return eastl::get<T>(val);
                } else {
                    panic("System variable '{}' is not of type '{}'", m_name, sv_supported_type_name<T>::value);
                }
            }
            reg[m_name] = get_fallback(); // Otherwise, set the fallback value and return it
            return eastl::get<T>(reg[m_name]);
        }

    private:
        [[nodiscard]] auto get_fallback() const -> T {
            if (eastl::holds_alternative<T>(m_fallback)) [[likely]]
                return eastl::get<T>(m_fallback);
            if (eastl::holds_alternative<eastl::function<auto() -> T>>(m_fallback))
                return eastl::invoke(eastl::get<eastl::function<auto() -> T>>(m_fallback));
            return T {};
        }

        const eastl::string m_name;
        const eastl::variant<eastl::monostate, T, eastl::function<auto() -> T>> m_fallback;
    };
}
