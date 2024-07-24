// Copyright (c) 2022 Mario "Neo" Sieg. All Rights Reserved.

#include "core.hpp"

#include <iostream>
#include <boxer/boxer.h>

namespace lu {
    static const auto main_tid = std::this_thread::get_id();

    auto panic_impl(std::string&& message) -> void {
        if (main_tid == std::this_thread::get_id()) // showing the message box is not thread safe
            boxer::show(message.c_str(), "Engine Error", boxer::Style::Error, boxer::Buttons::Quit);
        std::cerr << message << std::endl;
        std::abort();
    }
}
