// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../core/kernel.hpp"

#include "../platform/platform_subsystem.hpp"
#include "../graphics/graphics_subsystem.hpp"
#include "../scripting/scripting_subsystem.hpp"
#include "../physics/physics_subsystem.hpp"

static auto lunam_entry(const int argc, const char** argv, const char** environ) -> void {
    kernel kernel {argc, argv, environ};
    kernel.install<scripting::scripting_subsystem>();
    kernel.install<platform::platform_subsystem>();
    kernel.install<physics::physics_subsystem>();
    kernel.install<graphics::graphics_subsystem>();
    kernel.run();
}

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
auto __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int) -> int {
#else
auto main(const int argc, const char** argv, const char** environ) -> int {
#endif
    lunam_entry(argc, argv, environ);
    return EXIT_SUCCESS;
}
