// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "system_variable.hpp"

#include <filesystem>
#include <charconv>
#include <SimpleIni.h>

namespace soliton {
    static constexpr const char* cfg_file = "config/engine.ini";

    auto detail::save_system_variables() -> bool {
        if (!std::filesystem::exists("config")) {
            std::filesystem::create_directory("config");
            return false;
        }
        std::lock_guard lock {detail::sv_mutex};
        CSimpleIniA ini {};
        for (const auto& [name, value] : sv_registry) {
            if (eastl::holds_alternative<eastl::monostate>(value)) {
                continue;
            }
            const eastl::string& key = name;
            const eastl::string section = key.substr(0, key.find('.'));
            if (eastl::holds_alternative<bool>(value)) { // ugly but im foo lazy to lookup the variant visitor pattern rn
                ini.SetBoolValue(section.c_str(), key.c_str(), eastl::get<bool>(value));
            } else if (eastl::holds_alternative<float>(value)) {
                ini.SetDoubleValue(section.c_str(), key.c_str(), eastl::get<float>(value));
            } else if (eastl::holds_alternative<std::int64_t>(value)) {
                ini.SetLongValue(section.c_str(), key.c_str(), eastl::get<std::int64_t>(value));
            } else if (eastl::holds_alternative<eastl::string>(value)) {
                ini.SetValue(section.c_str(), key.c_str(), eastl::get<eastl::string>(value).c_str());
            }
        }
        if (ini.SaveFile(cfg_file) >= 0) [[likely]] {
            log_info("Saved {} system variables to '{}'", sv_registry.size(), cfg_file);
            return true;
        } else {
            log_error("Failed to save system variables to '{}'", cfg_file);
            return false;
        }
    }

    auto detail::load_system_variables() -> bool {
        if (!std::filesystem::exists("config")) [[unlikely]] {
            std::filesystem::create_directory("config");
            return false;
        }
        if (!std::filesystem::exists(cfg_file)) [[unlikely]] {
            log_warn("Config file '{}' not found, creating default", cfg_file);
            return false;
        }
        std::lock_guard lock {detail::sv_mutex};
        CSimpleIniA ini {};
        ini.SetUnicode();
        ini.LoadFile(cfg_file);
        CSimpleIniA::TNamesDepend sections {};
        ini.GetAllSections(sections);
        for (const auto& section : sections) {
            CSimpleIniA::TNamesDepend keys {};
            if (!ini.GetAllKeys(section.pItem, keys)) continue;
            for (const auto& key : keys) {
                const eastl::string value = ini.GetValue(section.pItem, key.pItem);
                if (value.empty()) {
                    log_warn("Empty value for key '{}'", key.pItem);
                    continue;
                }

                if (value == "true" || value == "false") {
                    sv_registry[key.pItem] = (value == "true");
                    continue;
                }

                std::int64_t intValue = 0;
                auto int_r = std::from_chars(value.data(), value.data() + value.size(), intValue);
                if (int_r.ec == std::errc() && int_r.ptr == value.data() + value.size()) {
                    sv_registry[key.pItem] = intValue;
                    continue;
                }

                #if COMPILER_CLANG && PLATFORM_OSX // Apple clang does not support from_chars for floats, use std::stof instead
                    char* end = nullptr;
                    float float_r = std::strtof(value.c_str(), &end);
                    if (end == value.data() + value.size()) {
                        sv_registry[key.pItem] = float_r;
                        continue;
                    }
                #else
                    float float_r = 0.0f;
                    auto floatResult = std::from_chars(value.data(), value.data() + value.size(), float_r);
                    if (floatResult.ec == std::errc() && floatResult.ptr == value.data() + value.size()) {
                        sv_registry[key.pItem] = float_r;
                        continue;
                    }
                #endif

                sv_registry[key.pItem] = value;
            }
        }
        log_info("Loaded {} system variables from '{}'", sv_registry.size(), cfg_file);
        return true;
    }
}
