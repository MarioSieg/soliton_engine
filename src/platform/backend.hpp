// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <GLFW/glfw3.h>

namespace lu::platform::backend {
    extern auto is_online() noexcept -> bool;
    extern auto init() -> void;
    extern auto shutdown() -> void;
    extern auto poll_events() -> void;
    extern auto print_monitor_info(GLFWmonitor* mon) -> void;
}
