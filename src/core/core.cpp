// Copyright (c) 2022 Mario "Neo" Sieg. All Rights Reserved.

#include "core.hpp"

#include <iostream>
#include <boxer/boxer.h>

static const auto kMainThreadID {std::this_thread::get_id()};

auto panic_impl(std::string&& message) -> void
try {
    if (kMainThreadID == std::this_thread::get_id()) // showing the message box is not thread safe
        boxer::show(message.c_str(), "Fatal Lunam Engine System Error", boxer::Style::Error, boxer::Buttons::Quit);
    std::cerr << message << std::endl;
    std::abort();
}
catch (...) {
    std::abort();
}
