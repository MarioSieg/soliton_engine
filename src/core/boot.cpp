// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "../core/kernel.hpp"
#include "../core/buffered_sink.hpp"

#include "../platform/platform_subsystem.hpp"
#include "../graphics/graphics_subsystem.hpp"
#include "../scripting/scripting_subsystem.hpp"
#include "../physics/physics_subsystem.hpp"
#include "../audio/audio_subsystem.hpp"
#include "../core/system_variable.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/fmt/chrono.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

static inline const eastl::string k_log_dir = "log";
static constexpr std::size_t k_log_threads = 1;
static constexpr std::size_t k_log_queue_size = 8192;

[[nodiscard]]
static auto create_logger(
    const eastl::string& name,
    const eastl::string& pattern,
    bool print_stdout = true,
    bool enroll = true
) -> std::shared_ptr<spdlog::logger> {
    const auto time = fmt::localtime(std::time(nullptr));
    eastl::vector<std::shared_ptr<spdlog::sinks::sink>> sinks {
        std::make_shared<soliton::buffered_sink>(k_log_queue_size),
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        fmt::format("{}/session {:%d-%m-%Y  %H-%M-%S}/{}.log", k_log_dir, time, name)),
    };
    if (print_stdout) {
        sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    }
    std::shared_ptr<spdlog::logger> result = std::make_shared<spdlog::async_logger>(
        name.c_str(),
        sinks.begin(),
        sinks.end(),
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::overrun_oldest
    );
    result->set_pattern(pattern.c_str());
    if (enroll) {
        register_logger(result);
    }
    return result;
}

static auto lunam_entry(const int argc, const char** argv, const char** $environ) -> void {
    using namespace soliton;

    std::ostream::sync_with_stdio(false);
    spdlog::init_thread_pool(k_log_queue_size, k_log_threads);
    std::shared_ptr<spdlog::logger> engine_logger = create_logger("engine", "%H:%M:%S:%e %s:%# %^[%l]%$ T:%t %v");
    std::shared_ptr<spdlog::logger> script_logger = create_logger("app", "%H:%M:%S:%e %v");
    spdlog::set_default_logger(engine_logger);
    log_info("-- ENGINE BOOT --");
    log_info("LunamEngine v.{}.{}", major_version(k_lunam_engine_version), minor_version(k_lunam_engine_version));
    log_info("Copyright (c) 2022-2024 Mario \"Neo\" Sieg. <mario.sieg.64@gmail.com> All Rights Reserved.");
    log_info("Log start date: {:%F %T}", fmt::localtime(std::time(nullptr)));
    log_info("Log dir: {}", k_log_dir);
    log_info("Booting Engine Kernel...");
    log_info("Build date: {}", __DATE__);
    log_info("Build time: {}", __TIME__);
    log_info("Loading global engine config...");
    detail::load_system_variables();

    {
        log_info("Initializing kernel...");
        kernel kernel {argc, argv, $environ};
        kernel.install<platform::platform_subsystem>();
        kernel.install<graphics::graphics_subsystem>();
        kernel.install<audio::audio_subsystem>();
        kernel.install<physics::physics_subsystem>();
        kernel.install<scripting::scripting_subsystem>();

        log_info("Running kernel...");
        kernel.run();
    }

    log_info("Saving global engine config...");
    detail::save_system_variables();
    log_info("Kernel exited...");
    spdlog::shutdown();
    std::cout.flush();
    std::fflush(stdout);
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
