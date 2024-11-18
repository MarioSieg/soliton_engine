// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "core.hpp"

#include <iostream>
#include <boxer/boxer.h>

namespace soliton {
    static const auto main_tid = std::this_thread::get_id();

    auto panic_impl(std::string&& message) -> void {
        if (main_tid == std::this_thread::get_id()) // showing the message box is not thread safe
            boxer::show(message.c_str(), "Fatal Engine Error", boxer::Style::Error, boxer::Buttons::Quit);
        std::cerr << message << std::endl;
        std::abort();
    }

    auto get_version_string() -> eastl::string {
        const auto v = unpack_version(k_lunam_engine_version);
        return fmt::format("{}.{}", v[0], v[1]).c_str();
    }
}
