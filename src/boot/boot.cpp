// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../core/kernel.hpp"

#include "../platform/subsystem.hpp"
#include "../graphics/subsystem.hpp"

static auto lunam_entry() -> void {
    kernel kernel {};
    kernel.install<platform::platform_subsystem>();
    kernel.install<graphics::graphics_subsystem>();
    kernel.run();
}

#if PLATFORM_WINDOWS
auto __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int) -> int {
#else
auto main(int, const char**) -> int {
#endif
    lunam_entry();
    return EXIT_SUCCESS;
}
