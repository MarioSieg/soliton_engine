// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "../core/kernel.hpp"

#include "../platform/platform_subsystem.hpp"
#include "../graphics/graphics_subsystem.hpp"
#include "../scripting/scripting_subsystem.hpp"
#include "../physics/physics_subsystem.hpp"
#include "../audio/audio_subsystem.hpp"
#include "../core/system_variable.hpp"

static auto lunam_entry(const int argc, const char** argv, const char** $environ) -> void {
    using namespace lu;

    bool sv_loaded = detail::load_system_variables();

    kernel kernel {argc, argv, $environ};
    kernel.install<platform::platform_subsystem>();
    kernel.install<graphics::graphics_subsystem>();
    kernel.install<audio::audio_subsystem>();
    kernel.install<physics::physics_subsystem>();
    kernel.install<scripting::scripting_subsystem>();

    if (!sv_loaded) detail::save_system_variables();

    kernel.run();

    if (!sv_loaded) detail::save_system_variables();
}

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
extern int __argc;
extern char** __argv;
auto __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int) -> int {
    const auto argc = __argc;
    const auto** argv = const_cast<const char**>(__argv);
    const auto** $environ = const_cast<const char**>(_environ);
#else
auto main(const int argc, const char** argv, const char** $environ) -> int {
#endif
    lunam_entry(argc, argv,  const_cast<const char**>($environ));
    return EXIT_SUCCESS;
}
