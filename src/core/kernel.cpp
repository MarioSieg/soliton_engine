// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "kernel.hpp"
#include "buffered_sink.hpp"

#include "../scene/scene.hpp"

#include <filesystem>
#include <iostream>

#if USE_MIMALLOC
#include <mimalloc.h>
#endif

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/fmt/chrono.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace lu {
    using namespace std::filesystem;

    static constexpr std::size_t k_log_threads = 1;
    static constexpr std::size_t k_log_queue_size = 8192;

    [[nodiscard]] static auto create_logger(const std::string& name, const std::string& pattern, bool print_stdout = true,
                                            bool enroll = true) -> std::shared_ptr<spdlog::logger> {
        const auto time = fmt::localtime(std::time(nullptr));
        std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks = {
                std::make_shared<buffered_sink>(k_log_queue_size),
                std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                        fmt::format("{}/session {:%d-%m-%Y  %H-%M-%S}/{}.log", kernel::log_dir, time, name)),
        };
        if (print_stdout) {
            sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        }
        std::shared_ptr<spdlog::logger> result = std::make_shared<spdlog::async_logger>(
                name,
                sinks.begin(),
                sinks.end(),
                spdlog::thread_pool(),
                spdlog::async_overflow_policy::overrun_oldest
        );
        result->set_pattern(pattern);
        if (enroll) {
            register_logger(result);
        }
        return result;
    }

#if PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
#include <fcntl.h>
#include <corecrt_io.h>
#include <Windows.h>
static auto redirect_io() -> void {
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
        return;
    HANDLE consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    int systemOutput = _open_osfhandle(std::bit_cast<std::intptr_t>(consoleOutput), _O_TEXT);
    if (!_isatty(systemOutput))
        return; // return if it's not a TTY
    FILE* cOutputHandle = _fdopen(systemOutput, "w");
    HANDLE consoleError = GetStdHandle(STD_ERROR_HANDLE);
    int systemError = _open_osfhandle(std::bit_cast<std::intptr_t>(consoleError), _O_TEXT);
    FILE* cErrorHandle = _fdopen(systemError, "w");
    HANDLE consoleInput = GetStdHandle(STD_INPUT_HANDLE);
    int systemInput = _open_osfhandle(std::bit_cast<std::intptr_t>(consoleInput), _O_TEXT);
    FILE* cInputHandle = _fdopen(systemInput, "r");
    std::ios::sync_with_stdio(true);
    freopen_s(&cInputHandle, "CONIN$", "r", stdin);
    freopen_s(&cOutputHandle, "CONOUT$", "w", stdout);
    freopen_s(&cErrorHandle, "CONOUT$", "w", stderr);
    std::wcout.clear();
    std::cout.clear();
    std::wcerr.clear();
    std::cerr.clear();
    std::wcin.clear();
    std::cin.clear();
}
#else
#include <pthread.h>
#endif

    static constinit kernel* g_kernel = nullptr;

    kernel::kernel(const int argc, const char** argv, const char** $environ) {
        passert(g_kernel == nullptr);
        g_kernel = this;
#if PLATFORM_WINDOWS
        redirect_io();
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#elif PLATFORM_LINUX
        pthread_t cthr_id = pthread_self();
    pthread_setname_np(cthr_id, "Lunam Engine Main Thread");
    pthread_attr_t thr_attr {};
    int policy = 0;
    int max_prio_for_policy = 0;
    pthread_attr_init(&thr_attr);
    pthread_attr_getschedpolicy(&thr_attr, &policy);
    max_prio_for_policy = sched_get_priority_max(policy);
    pthread_setschedprio(cthr_id, max_prio_for_policy);
    pthread_attr_destroy(&thr_attr);
#endif
        std::ostream::sync_with_stdio(false);
        spdlog::init_thread_pool(k_log_queue_size, k_log_threads);
        std::shared_ptr<spdlog::logger> engineLogger = create_logger("engine", "%H:%M:%S:%e %s:%# %^[%l]%$ T:%t %v");
        std::shared_ptr<spdlog::logger> scriptLogger = create_logger("app", "%H:%M:%S:%e %v");
        spdlog::set_default_logger(engineLogger);
        log_info("LunamEngine v.{}.{}", major_version(k_lunam_engine_v), minor_version(k_lunam_engine_v));
        log_info("Copyright (c) 2022-2024 Mario \"Neo\" Sieg. All Rights Reserved.");
        log_info("Booting Engine Kernel...");
        log_info("Build date: {}", __DATE__);
        log_info("Build time: {}", __TIME__);
#if USE_MIMALLOC
        log_info("Allocator version: {:#X}", mi_version());
#endif
        log_info("Working dir: {}", std::filesystem::current_path().string());
        log_info("ARG VEC");
        for (int i = 0; i < argc; ++i) {
            log_info("  {}: {}", i, argv[i]);
        }
        log_info("ENVIRON VEC");
        for (int i = 0; $environ[i] != nullptr; ++i) {
            log_info("  {}: {}", i, $environ[i]);
        }
        log_info("Engine config dir: {}", config_dir);
        log_info("Engine log dir: {}", log_dir);

        assetmgr::init();
    }

    kernel::~kernel() {
        log_info("Shutting down...");
        log_info("Killing active scene...");
        // Kill active scene before other subsystems are shut down
        scene::s_active.reset();
        log_info("Killing subsystems...");
        m_subsystems.clear();
        log_info("Patching core engine config...");
        assetmgr::shutdown();
#if USE_MIMALLOC
        mi_stats_merge();
        static std::stringstream ss;
        mi_stats_print_out(+[](const char* msg, [[maybe_unused]] void* ud) {
            ss << msg;
        }, nullptr);
        log_info("Allocator Stats:\n{}", ss.str());
#endif
        log_info("System offline");
        spdlog::shutdown();
        std::cout << "Lunam Engine says goodbye :(\n";
        std::cout.flush();
        std::fflush(stdout);
        g_kernel = nullptr;
    }

    static constinit double g_delta_time, g_time;

    static auto compute_delta_time() noexcept {
        static auto prev = std::chrono::high_resolution_clock::now();
        auto now = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(now - prev);
        g_delta_time = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();
        prev = now;
        g_time += g_delta_time;
    }

    auto kernel::get() noexcept -> kernel& {
        passert(g_kernel != nullptr);
        return *g_kernel;
    }

    auto kernel::get_delta_time() noexcept -> double {
        return g_delta_time;
    }

    auto kernel::get_time() noexcept -> double {
        return g_time;
    }

    static constinit bool g_kernel_online = true;
    auto kernel::request_exit() noexcept -> void {
        g_kernel_online = false;
    }

    auto kernel::on_new_scene_start(scene& scene) -> void {
        std::ranges::for_each(m_subsystems, [&scene](const std::shared_ptr<subsystem>& sys) {
            sys->on_start(scene);
        });
    }

    HOTPROC auto kernel::run() -> void {
        std::ranges::for_each(m_subsystems, [](const std::shared_ptr<subsystem>& sys) {
            sys->on_prepare();
        });

        const auto now = std::chrono::high_resolution_clock::now();
        log_info("Boot time: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(now - boot_stamp).count());

        // simulation loop
        while (tick());
    }

    HOTPROC auto kernel::tick() -> bool {
        bool flag = g_kernel_online;
        compute_delta_time();
        std::for_each(m_subsystems.cbegin(), m_subsystems.cend(), [&flag](const std::shared_ptr<subsystem>& sys) {
            //scoped_profiler_sample ps {std::string{sys->name}};
            if (!sys->on_pre_tick()) [[unlikely]]
                flag = false;
        });
        std::for_each(m_subsystems.cbegin(), m_subsystems.cend(), [](const std::shared_ptr<subsystem>& sys) {
            sys->on_tick();
        });
        std::for_each(m_subsystems.crbegin(), m_subsystems.crend(), [](const std::shared_ptr<subsystem>& sys) {
            sys->on_post_tick();
        });
        ++m_frame;
        return flag;
    }

    auto kernel::resize() -> void {
        std::ranges::for_each(m_subsystems, [](const std::shared_ptr<subsystem>& sys) {
            sys->on_resize();
        });
    }
}
