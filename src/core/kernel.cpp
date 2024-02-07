// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "kernel.hpp"
#include "../scene/scene.hpp"

#include <iostream>

#include <mimalloc.h>

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/fmt/chrono.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

static constexpr std::size_t k_log_threads = 1;
static constexpr std::size_t k_log_queue_size = 8192;

class buf_sink final : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit buf_sink(std::size_t cap) { m_Backtrace.reserve(cap); }

    auto get() const noexcept -> std::span<const std::pair<spdlog::level::level_enum, std::string>> {
        return m_Backtrace;
    }

    auto clear() noexcept -> void { m_Backtrace.clear(); }

private:
    std::vector<std::pair<spdlog::level::level_enum, std::string>> m_Backtrace{};

    auto sink_it_(const spdlog::details::log_msg& msg) -> void override {
        spdlog::memory_buf_t buffer{};
        formatter_->format(msg, buffer);
        std::string message = {buffer.data(), buffer.size()};
        m_Backtrace.emplace_back(std::make_pair(msg.level, std::move(message)));
    }

    auto flush_() -> void override {}
};

[[nodiscard]] static auto create_logger(const std::string& name, const std::string& pattern, bool print_stdout = true,
                                       bool enroll = true) -> std::shared_ptr<spdlog::logger> {
    const auto time = fmt::localtime(std::time(nullptr));
    std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks = {
        //std::make_shared<buf_sink>(k_log_queue_size),
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            fmt::format("log/session {:%d-%m-%Y  %H-%M-%S}/{}.log", time, name)),
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
#endif

static constinit kernel* g_kernel = nullptr;

kernel::kernel() {
    passert(g_kernel == nullptr);
    g_kernel = this;
#if PLATFORM_WINDOWS
    redirect_io();
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS); // Is this smart?
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif
    std::ostream::sync_with_stdio(false);
    spdlog::init_thread_pool(k_log_queue_size, k_log_threads);
    std::shared_ptr<spdlog::logger> engineLogger = create_logger("engine", "%H:%M:%S:%e %s:%# %^[%l]%$ T:%t %v");
    std::shared_ptr<spdlog::logger> scriptLogger = create_logger("app", "%H:%M:%S:%e %v");
    spdlog::set_default_logger(engineLogger);
    log_info("LunamEngine v0.0.1"); // TODO: version
    log_info("Copyright (c) 2022-2023 Mario \"Neo\" Sieg. All Rights Reserved.");
    log_info("Booting Engine Kernel...");
    log_info("Build date: {}", __DATE__);
    log_info("Build time: {}", __TIME__);
    log_info("MIMAL version: {:#X}", mi_version());
}

kernel::~kernel() {
    log_info("Shutting down...");
    log_info("Killing active scene...");
    // Kill active scene before other subsystems are shut down
    auto& active = const_cast<std::unique_ptr<scene>&>(scene::get_active());
    active.reset();
    log_info("Killing subsystems...");
    m_subsystems.clear();
    // Print asset manager infos
    log_info("Asset manager stats:");
    log_info("  Total assets requests: {}", assetmgr::get_asset_request_count());
    log_info("  Total data loaded: {:.03f} MiB", static_cast<double>(assetmgr::get_total_bytes_loaded()) / (1024.0*1024.0));
    log_info("System offline");
    std::cout.flush();
    std::fflush(stdout);
    g_kernel = nullptr;
}

static constinit double delta_time;
static auto compute_delta_time() noexcept {
    static auto prev = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(now - prev);
    delta_time = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();
    prev = now;
}

auto kernel::get() noexcept -> kernel& {
    passert(g_kernel != nullptr);
    return *g_kernel;
}

auto kernel::get_delta_time() noexcept -> double {
    return delta_time;
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
    while(tick()) [[likely]] {}
}

HOTPROC auto kernel::tick() const -> bool {
    bool flag = g_kernel_online;
    compute_delta_time();
    std::for_each(m_subsystems.cbegin(), m_subsystems.cend(), [&flag](const std::shared_ptr<subsystem>& sys) {
        if (!sys->on_pre_tick()) [[unlikely]]
            flag = false;
    });
    std::for_each(m_subsystems.cbegin(), m_subsystems.cend(), [](const std::shared_ptr<subsystem>& sys) {
        sys->on_tick();
    });
    std::for_each(m_subsystems.crbegin(), m_subsystems.crend(), [](const std::shared_ptr<subsystem>& sys) {
       sys->on_post_tick();
    });
    return flag;
}

auto kernel::resize() -> void {
    std::ranges::for_each(m_subsystems, [](const std::shared_ptr<subsystem>& sys) {
       sys->on_resize();
   });
}
