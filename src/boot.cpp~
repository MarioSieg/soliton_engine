// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include <memory>
#include <span>
#include <vector>

#include "core.hpp"

#include <GLFW/glfw3.h>
#include <mimalloc.h>
#include <infoware/infoware.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/fmt/chrono.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#if LU_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdlib>
#include <fcntl.h>
#include <io.h>
#include <psapi.h>
#include <iostream>
#else
#include <link.h>
#endif

static constexpr std::size_t kLogThreads = 1;
static constexpr std::size_t kLogQueueSize = 8192;

class BufferedSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit BufferedSink(std::size_t capacity) { m_Backtrace.reserve(capacity); }

    auto GetBacktrace() const noexcept -> std::span<const std::pair<spdlog::level::level_enum, std::string>> {
        return m_Backtrace;
    }

    auto Clear() noexcept -> void { m_Backtrace.clear(); }

private:
    std::vector<std::pair<spdlog::level::level_enum, std::string>> m_Backtrace{};

    auto sink_it_(const spdlog::details::log_msg& msg) -> void override {
        spdlog::memory_buf_t buffer{};
        formatter_->format(msg, buffer);
        std::string message = {buffer.data(), buffer.size()};
        m_Backtrace.emplace_back(std::make_pair(msg.level, std::move(message)));
    }

    auto flush_() -> void override {
    }
};

[[nodiscard]] static auto CreateLogger(const std::string& name, const std::string& pattern, bool printStdout = true,
                                       bool doRegister = true) -> std::shared_ptr<spdlog::logger> {
    const auto time = fmt::localtime(std::time(nullptr));
    std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks = {
        std::make_shared<BufferedSink>(kLogQueueSize),
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            fmt::format("log/session {:%d-%m-%Y  %H-%M-%S}/{}.log", time, name)),
    };
    if (printStdout) {
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
    if (doRegister) {
        register_logger(result);
    }
    return result;
}

static constexpr auto KernelVariantName(const iware::system::kernel_t variant) noexcept -> std::string_view {
    switch (variant) {
    case iware::system::kernel_t::windows_nt:
        return "Windows NT";
    case iware::system::kernel_t::linux:
        return "Linux";
    case iware::system::kernel_t::darwin:
        return "Darwin";
    default:
        return "unknown";
    }
}

static constexpr auto CacheTypeName(const iware::cpu::cache_type_t cache_type) noexcept -> std::string_view {
    switch (cache_type) {
    case iware::cpu::cache_type_t::unified:
        return "unified";
    case iware::cpu::cache_type_t::instruction:
        return "instruction";
    case iware::cpu::cache_type_t::data:
        return "data";
    case iware::cpu::cache_type_t::trace:
        return "trace";
    default:
        return "unknown";
    }
}

static constexpr auto ArchitectureName(const iware::cpu::architecture_t architecture) noexcept -> std::string_view {
    switch (architecture) {
    case iware::cpu::architecture_t::x64:
        return "x64";
    case iware::cpu::architecture_t::arm:
        return "ARM";
    case iware::cpu::architecture_t::itanium:
        return "Itanium";
    case iware::cpu::architecture_t::x86:
        return "x86";
    default:
        return "unknown";
    }
}

static constexpr auto EndiannessName(const iware::cpu::endianness_t endianness) noexcept -> std::string_view {
    switch (endianness) {
    case iware::cpu::endianness_t::little:
        return "little-endian";
    case iware::cpu::endianness_t::big:
        return "big-endian";
    default:
        return "unknown";
    }
}

#if LU_CPU_X86
static constexpr std::array<std::string_view, 38> kAMD64Extensions = {
    "3DNow",
    "3DNow Extended",
    "MMX",
    "MMX Extended",
    "SSE",
    "SSE 2",
    "SSE 3",
    "SSSE 3",
    "SSE 4A",
    "SSE 41",
    "SSE 42",
    "AES",
    "AVX",
    "AVX2",
    "AVX 512",
    "AVX 512 F",
    "AVX 512 CD",
    "AVX 512 PF",
    "AVX 512 ER",
    "AVX 512 VL",
    "AVX 512 BW",
    "AVX 512 BQ",
    "AVX 512 DQ",
    "AVX 512 IFMA",
    "AVX 512 VBMI",
    "HLE",
    "BMI1",
    "BMI2",
    "ADX",
    "MPX",
    "SHA",
    "PrefetchWT1",
    "FMA3",
    "FMA4",
    "XOP",
    "RDRand",
    "X64",
    "X87 FPU"
};
static_assert(kAMD64Extensions.size() == static_cast<std::size_t>(iware::cpu::instruction_set_t::x87_fpu) + 1);
#endif

auto DumpOSInfo() -> void {
    const auto osInfo = iware::system::OS_info();
    LU_LOG_INFO("OS: {}, v.{}.{}.{}.{}", osInfo.name, osInfo.major, osInfo.minor, osInfo.patch, osInfo.build_number);
}

auto DumpCPUInfo() -> void {
    std::string name = iware::cpu::model_name();
    std::string vendor = iware::cpu::vendor();
    std::string vendorId = iware::cpu::vendor_id();
    auto [logical, physical, packages] = iware::cpu::quantities();
    std::string_view arch = ArchitectureName(iware::cpu::architecture());
    double baseFrequencyGHz = static_cast<double>(iware::cpu::frequency()) / 1e9;
    std::string_view endianness = EndiannessName(iware::cpu::endianness());
    iware::cpu::cache_t l1Cache = iware::cpu::cache(1);
    iware::cpu::cache_t l2Cache = iware::cpu::cache(2);
    iware::cpu::cache_t l3Cache = iware::cpu::cache(3);

    LU_LOG_INFO("CPU(s): {} X {}", packages, name);
    LU_LOG_INFO("CPU Architecture: {}", arch);
    LU_LOG_INFO("CPU Base frequency: {:.02F} ghz", baseFrequencyGHz);
    LU_LOG_INFO("CPU Endianness: {}",  endianness);
    LU_LOG_INFO("CPU Logical cores: {}", logical);
    LU_LOG_INFO("CPU Physical cores: {}", physical);
    LU_LOG_INFO("CPU Sockets: {}", packages);
    LU_LOG_INFO("CPU Vendor: {}", vendor);
    LU_LOG_INFO("CPU Vendor ID: {}", vendorId);
    std::size_t hwc = std::thread::hardware_concurrency();
    LU_LOG_INFO("Hardware concurrency: {}, status: {}", hwc, hwc >= 8 ? "GOOD" : "BAD");

    static constexpr auto dumpCache = [](unsigned level, const iware::cpu::cache_t& cache) {
        const auto [size, line_size, associativity, type] {cache};
        LU_LOG_INFO("L{} Cache size: {} KiB", level, static_cast<double>(size) / 1024.0);
        LU_LOG_INFO("L{} Cache line size: {} B", level, line_size);
        LU_LOG_INFO("L{} Cache associativity: {}", level, associativity);
        LU_LOG_INFO("L{} Cache type: {}", level, CacheTypeName(type));
    };

    dumpCache(1, l1Cache);
    dumpCache(2, l2Cache);
    dumpCache(3, l3Cache);

    for (std::size_t i = 0; i < kAMD64Extensions.size(); ++i) {
        bool supported = instruction_set_supported(static_cast<iware::cpu::instruction_set_t>(i));
        LU_LOG_INFO("{: <16} [{}]", kAMD64Extensions[i], supported ? 'X' : ' ');
    }
}

auto DumpMemoryInfo() -> void {
    const double kGiB = std::pow(1024.0, 3.0);
    const auto mem = iware::system::memory();
    LU_LOG_INFO("RAM physical total: {:.04F} GiB", static_cast<double>(mem.physical_total) / kGiB);
    LU_LOG_INFO("RAM physical available: {:.04F} GiB", static_cast<double>(mem.physical_available) / kGiB);
    LU_LOG_INFO("RAM virtual total: {:.04F} GiB", static_cast<double>(mem.virtual_total) / kGiB);
    LU_LOG_INFO("RAM virtual available: {:.04F} GiB", static_cast<double>(mem.virtual_available) / kGiB);
}

auto DumpLoadedDylibs() -> void {
#if LU_PLATFORM_WINDOWS
    std::array<HMODULE, 8192 << 2> hMods {};
    HANDLE hProcess = GetCurrentProcess();
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hMods.data(), sizeof hMods, &cbNeeded)) {
        for (UINT i = 0; i < cbNeeded / sizeof(HMODULE); ++i) {
            std::array<TCHAR, MAX_PATH> szModName {};
            if (GetModuleFileNameEx(hProcess, hMods[i], szModName.data(), sizeof(szModName) / sizeof(TCHAR))) {
                LU_LOG_INFO("{}", szModName.data());
            }
        }
    }
#else
    dl_iterate_phdr(+[](const dl_phdr_info* info, std::size_t, void*) -> int {
        LU_LOG_INFO("{}", info->dlpi_name);
        return 0;
    }, nullptr);
#endif
}

static constinit std::atomic_bool s_Initialized = false;
static constinit GLFWwindow* s_Window = nullptr;

static auto InitPlatformBackend() -> void {
    LU_LOG_INFO("Initializing platform backend");
    LU_Assert(!s_Initialized.load(std::memory_order_seq_cst));
    LU_Assert(glfwInit() == GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // window is hidden by default
    s_Window = glfwCreateWindow(1280, 720, "LunamEngine", nullptr, nullptr);
    s_Initialized.store(true, std::memory_order_seq_cst);
}

static auto ShutdownPlatformBackend() -> void {
    LU_Assert(s_Initialized.load(std::memory_order_seq_cst));
    glfwDestroyWindow(s_Window);
    glfwTerminate();
}

static auto IsSimRunning() -> bool {
    LU_Assert(s_Initialized.load(std::memory_order_seq_cst));
    return !glfwWindowShouldClose(s_Window);
}

static auto TickSim() -> void {
    glfwPollEvents();
}

static auto EnterSimLoop() -> void {
    LU_Assert(s_Initialized.load(std::memory_order_seq_cst));
    glfwShowWindow(s_Window); // show the window
    while (IsSimRunning()) [[likely]] {
        TickSim();
    }
}

static auto PrintSep() -> void {
    LU_LOG_INFO("------------------------------------------------------------");
}

static auto LunamMain() -> void {
    const auto clock = std::chrono::system_clock::now();
    std::ostream::sync_with_stdio(false);
    spdlog::init_thread_pool(kLogQueueSize, kLogThreads);
    auto engineLogger = CreateLogger("LunamEngine", "%H:%M:%S:%e %s:%# %^[%l]%$ T:%t %v");
    auto scriptLogger = CreateLogger("Lua", "%H:%M:%S:%e %v");
    set_default_logger(engineLogger);
    LU_LOG_INFO("LunamEngine v0.0.1");
    LU_LOG_INFO("Booting Engine Kernel...");
    LU_LOG_INFO("Build date: {}", __DATE__);
    LU_LOG_INFO("Build time: {}", __TIME__);
    LU_LOG_INFO("MIMAL version: {:#X}", mi_version());
    PrintSep();
    DumpOSInfo();
    PrintSep();
    DumpCPUInfo();
    PrintSep();
    DumpMemoryInfo();
    PrintSep();
    DumpLoadedDylibs();
    PrintSep();
    InitPlatformBackend();
    LU_LOG_INFO("Engine online in {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - clock).count());
    LU_LOG_INFO("Entering simulation");
    EnterSimLoop();
    LU_LOG_INFO("Shutting down...");
    ShutdownPlatformBackend();
    LU_LOG_INFO("System offline");
}

#if LU_PLATFORM_WINDOWS
static auto RedirectIoToConsole() -> void {
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
auto __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int) -> int {
    RedirectIoToConsole();
#else
auto main(int, const char**) -> int {
#endif
    LunamMain();
    std::cout.flush();
    std::fflush(stdout);
    return EXIT_SUCCESS;
}
