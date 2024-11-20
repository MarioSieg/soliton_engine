// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "kernel.hpp"
#include "../scene/scene_mgr.hpp"

#include <bit>
#include <ranges>
#include <filesystem>
#include <iostream>

#if USE_MIMALLOC
#include <mimalloc.h>
#endif

// Ensure that the application uses the dedicated GPU instead of the integrated GPU. Drivers searches for these variables.
#if PLATFORM_WINDOWS
extern "C" __declspec(dllexport) unsigned __int32 NvOptimusEnablement = 0x00000001;
extern "C" __declspec(dllexport) unsigned __int32 AmdPowerXpressRequestHighPerformance = 0x00000001;
#endif

namespace soliton {
    using namespace std::filesystem;
    using namespace eastl::chrono;

    static constinit double g_delta_time, g_time;
    static constinit bool g_kernel_online = true;

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <fcntl.h>
#include <corecrt_io.h>
#include <Windows.h>
static auto redirect_io() -> void {
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
        return;
    HANDLE consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    int systemOutput = _open_osfhandle(eastl::bit_cast<std::intptr_t>(consoleOutput), _O_TEXT);
    if (!_isatty(systemOutput))
        return; // return if it's not a TTY
    FILE* cOutputHandle = _fdopen(systemOutput, "w");
    HANDLE consoleError = GetStdHandle(STD_ERROR_HANDLE);
    int systemError = _open_osfhandle(eastl::bit_cast<std::intptr_t>(consoleError), _O_TEXT);
    FILE* cErrorHandle = _fdopen(systemError, "w");
    HANDLE consoleInput = GetStdHandle(STD_INPUT_HANDLE);
    int systemInput = _open_osfhandle(eastl::bit_cast<std::intptr_t>(consoleInput), _O_TEXT);
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
        panic_assert(g_kernel == nullptr);
        g_kernel = this;
#if PLATFORM_WINDOWS
        redirect_io();
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
        SetConsoleTitleA("Soliton Engine Main Thread");
#elif PLATFORM_LINUX
        pthread_t cthr_id = pthread_self();
        pthread_setname_np(cthr_id, "Soliton Engine Main Thread");
        pthread_attr_t thr_attr {};
        int policy = 0;
        int max_prio_for_policy = 0;
        pthread_attr_init(&thr_attr);
        pthread_attr_getschedpolicy(&thr_attr, &policy);
        max_prio_for_policy = sched_get_priority_max(policy);
        pthread_setschedprio(cthr_id, max_prio_for_policy);
        pthread_attr_destroy(&thr_attr);
#elif PLATFORM_OSX
        setpriority(PRIO_PROCESS, 0, -10);  // Use -5 to -10 instead of -20 for a better balance
        pthread_t cthr_id = pthread_self();
        pthread_setname_np("Soliton Engine Main Thread");
        pthread_attr_t thr_attr {};
        int policy = 0;
        int max_prio_for_policy = 0;
        pthread_attr_init(&thr_attr);
        pthread_attr_getschedpolicy(&thr_attr, &policy);
        max_prio_for_policy = sched_get_priority_max(policy);
        sched_param param;
        param.sched_priority = max_prio_for_policy - 5;  // Set it to a reasonable level
        pthread_setschedparam(cthr_id, policy, &param);
        pthread_attr_destroy(&thr_attr);
#endif
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

        assetmgr::init(); // Initialize asset manager

        log_info("-- ENGINE KERNEL BOOT DONE {:.03f}s --", duration_cast<duration<double>>(duration_cast<high_resolution_clock::duration>(high_resolution_clock::now() - m_boot_stamp)).count());
    }

    kernel::~kernel() {
        log_info("Shutting down...");
        log_info("Killing active scene...");
        // Kill active scene before other subsystems are shut down
        scene_mgr::set_active(nullptr);
        log_info("Killing subsystems...");
        m_subsystems.clear();
        log_info("Patching core engine config...");
        assetmgr::shutdown();
#if USE_MIMALLOC
        mi_stats_merge();
        std::stringstream ss {};
        mi_stats_print_out(+[](const char* msg, void* ud) {
            *static_cast<std::stringstream*>(ud) << msg;
        }, &ss);
        log_info("--------- Engine Memory Allocator Stats ---------");
        for (std::string line; std::getline(ss, line); )
            log_info("\t{}", line);
#endif
        log_info("System offline");
        g_kernel = nullptr;
    }

    auto kernel::get() noexcept -> kernel& {
        panic_assert(g_kernel != nullptr);
        return *g_kernel;
    }

    auto kernel::get_delta_time() noexcept -> double {
        return g_delta_time;
    }

    auto kernel::get_time() noexcept -> double {
        return g_time;
    }

    auto kernel::request_exit() noexcept -> void {
        g_kernel_online = false;
    }

    auto kernel::start_scene(scene& scene) -> void {
        stopwatch clock_total {};
        log_info("Starting new scene...");
        for (auto&& sys : m_subsystems) {
            const stopwatch clock {};
            sys->on_start(scene);
            sys->m_prev_on_start_time = clock.elapsed<nanoseconds>();
            log_info("Scene start time of subsystem '{}': {:.03f} s", sys->name, duration_cast<duration<double>>(sys->m_prev_on_start_time).count());
        }
        log_info("Scene start time of engine: {:.03f} s", clock_total.elapsed_secs());
    }

    extern eastl::unique_ptr<scene> s_active_scene;

    HOTPROC auto kernel::run() -> void {
        for (auto&& sys : m_subsystems) {
            const stopwatch clock {};
            sys->on_prepare();
            sys->m_prepare_time = clock.elapsed<nanoseconds>();
        }
        if (s_active_scene) {
            s_active_scene->on_start(); // Start the active scene
        } else {
            scene_mgr::set_active(eastl::make_unique<scene>()); // Create a new empty scene, started by set_active
        }
        for (auto&& sys : m_subsystems) {
            const double boot_time = eastl::chrono::duration_cast<eastl::chrono::duration<double>>(sys->total_startup_time()).count();
            log_info("Startup time of subsystem '{}' (boot + prepare): {:.03f} s", sys->name, boot_time);
            constexpr double startup_time_lim = 3.0;
            if (boot_time > startup_time_lim) {
                log_warn("Startup time of subsystem '{}' (boot + prepare): {:.03f} s exceeds recommended startup time: {} s. Optimize startup routines to ensure fast boot time!", sys->name, boot_time, startup_time_lim);
            }
        }
        log_info("Total boot time: {:.03f} s", duration_cast<duration<double>>(high_resolution_clock::now() - m_boot_stamp).count());
        while (tick()) [[likely]]; // simulation loop
    }

    HOTPROC auto kernel::tick() -> bool {
        static time_point prev = high_resolution_clock::now();
        const time_point now = high_resolution_clock::now();
        g_delta_time = duration_cast<duration<double>>(duration_cast<high_resolution_clock::duration>(now - prev)).count();
        prev = now;
        g_time += g_delta_time;
        for (auto&& sys : m_subsystems) {
            const stopwatch clock {};
            if (!sys->on_pre_tick()) [[unlikely]] return false;
            sys->m_prev_pre_tick_time = clock.elapsed<nanoseconds>();
        }
        for (auto&& sys : m_subsystems) {
            const stopwatch clock {};
            sys->on_tick();
            sys->m_prev_tick_time = clock.elapsed<nanoseconds>();
        }
        for (auto&& i = m_subsystems.rbegin(); i != m_subsystems.rend(); ++i) {
            const auto& sys = *i;
            const stopwatch clock {};
            sys->on_post_tick();
            sys->m_prev_post_tick_time = clock.elapsed<nanoseconds>();
        }
        ++m_frame;
        return g_kernel_online;
    }

    auto kernel::resize() -> void {
        stopwatch clock_total {};
        for (auto&& sys : m_subsystems) {
            stopwatch clock {};
            sys->on_resize();
            sys->m_prev_resize_time = clock.elapsed<nanoseconds>();
            log_info("Resize time of subsystem '{}': {:.03f} s", sys->name, duration_cast<duration<double>>(sys->m_prev_resize_time).count());
        }
        log_info("Resize time of engine: {:.03f} s", clock_total.elapsed_secs());
    }
}
