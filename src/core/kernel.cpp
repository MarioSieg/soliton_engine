// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "kernel.hpp"

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
        std::make_shared<buf_sink>(k_log_queue_size),
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
#include <fcntl.h>
#include <processthreadsapi.h>
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

kernel::kernel() {
#if PLATFORM_WINDOWS
    redirect_io();
    SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS); // Is this smart?
#endif
    std::ostream::sync_with_stdio(false);
    spdlog::init_thread_pool(k_log_queue_size, k_log_threads);
    std::shared_ptr<spdlog::logger> engineLogger = create_logger("Engine", "%H:%M:%S:%e %s:%# %^[%l]%$ T:%t %v");
    std::shared_ptr<spdlog::logger> scriptLogger = create_logger("App", "%H:%M:%S:%e %v");
    spdlog::set_default_logger(engineLogger);
    log_info("LunamEngine v0.0.1");
    log_info("Copyright (c) 2022-2023 Mario \"Neo\" Sieg. All Rights Reserved.");
    log_info("Booting Engine Kernel...");
    log_info("Build date: {}", __DATE__);
    log_info("Build time: {}", __TIME__);
    log_info("MIMAL version: {:#X}", mi_version());
}

kernel::~kernel() {
    log_info("Shutting down...");
    m_subsystems.clear();
    log_info("System offline");
    std::cout.flush();
    std::fflush(stdout);
}

auto kernel::run() -> void {
    std::ranges::for_each(m_subsystems, [](const std::shared_ptr<subsystem>& sys) {
        sys->on_prepare();
    });

    const auto now = std::chrono::high_resolution_clock::now();
    log_info("Boot time: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(now - boot_stamp).count());

    volatile bool running = true;

    // simulation loop
    do {
        std::for_each(m_subsystems.cbegin(), m_subsystems.cend(), [&running](const std::shared_ptr<subsystem>& sys) {
            if (!sys->on_pre_tick()) [[unlikely]]
                running = false;
        });

        std::for_each(m_subsystems.crbegin(), m_subsystems.crend(), [](const std::shared_ptr<subsystem>& sys) {
           sys->on_post_tick();
        });
    } while(running);
}

auto kernel::resize() -> void {
    std::ranges::for_each(m_subsystems, [](const std::shared_ptr<subsystem>& sys) {
       sys->on_resize();
   });
}