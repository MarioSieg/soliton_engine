// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "subsystem.hpp"

#include <iostream>
#include <filesystem>

#include "..\graphics\subsystem.hpp"

#include <GLFW/glfw3.h>
#if PLATFORM_WINDOWS // TODO: OSX, Linux
#define GLFW_EXPOSE_NATIVE_WIN32
#include <Windows.h>
#include <Lmcons.h>
#include <processthreadsapi.h>
#include <cstdlib>
#include <fcntl.h>
#include <io.h>
#include <psapi.h>
#endif
#include <GLFW/glfw3native.h>
#include <mimalloc.h>
#include <mimalloc-new-delete.h>
#include <infoware/infoware.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace platform {
static constexpr auto kernel_variant_name(iware::system::kernel_t variant) noexcept -> std::string_view {
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

static constexpr auto cache_type_name(iware::cpu::cache_type_t cache_type) noexcept -> std::string_view {
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

static constexpr auto architecture_name(iware::cpu::architecture_t architecture) noexcept -> std::string_view {
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

static constexpr auto endianness_name(const iware::cpu::endianness_t endianness) noexcept -> std::string_view {
    switch (endianness) {
    case iware::cpu::endianness_t::little:
        return "little-endian";
    case iware::cpu::endianness_t::big:
        return "big-endian";
    default:
        return "unknown";
    }
}

#if CPU_X86 // TODO: AArch64
static constexpr std::array<std::string_view, 38> k_amd64_extensions = {
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
static_assert(k_amd64_extensions.size() == static_cast<std::size_t>(iware::cpu::instruction_set_t::x87_fpu) + 1);
#endif

auto dump_os_info() -> void {
    const auto osInfo = iware::system::OS_info();
    log_info("OS: {}, v.{}.{}.{}.{}", osInfo.full_name, osInfo.major, osInfo.minor, osInfo.patch, osInfo.build_number);
#if PLATFORM_WINDOWS
    TCHAR username[UNLEN+1];
    DWORD username_len = UNLEN+1;
    GetUserName(username, &username_len);
    log_info("Username: {}", username);
#endif
}

auto dump_cpu_info() -> void {
    std::string name = iware::cpu::model_name();
    std::string vendor = iware::cpu::vendor();
    std::string vendorId = iware::cpu::vendor_id();
    auto [logical, physical, packages] = iware::cpu::quantities();
    std::string_view arch = architecture_name(iware::cpu::architecture());
    double baseFrequencyGHz = static_cast<double>(iware::cpu::frequency()) / 1e9;
    std::string_view endianness = endianness_name(iware::cpu::endianness());
    iware::cpu::cache_t l1Cache = iware::cpu::cache(1);
    iware::cpu::cache_t l2Cache = iware::cpu::cache(2);
    iware::cpu::cache_t l3Cache = iware::cpu::cache(3);

    log_info("CPU(s): {} X {}", packages, name);
    log_info("CPU Architecture: {}", arch);
    log_info("CPU Base frequency: {:.02F} ghz", baseFrequencyGHz);
    log_info("CPU Endianness: {}",  endianness);
    log_info("CPU Logical cores: {}", logical);
    log_info("CPU Physical cores: {}", physical);
    log_info("CPU Sockets: {}", packages);
    log_info("CPU Vendor: {}", vendor);
    log_info("CPU Vendor ID: {}", vendorId);
    std::size_t hwc = std::thread::hardware_concurrency();
    log_info("Hardware concurrency: {}, status: {}", hwc, hwc >= 8 ? "GOOD" : "BAD");

    static constexpr auto dumpCache = [](unsigned level, const iware::cpu::cache_t& cache) {
        const auto [size, line_size, associativity, type] {cache};
        log_info("L{} Cache size: {} KiB", level, static_cast<double>(size) / 1024.0);
        log_info("L{} Cache line size: {} B", level, line_size);
        log_info("L{} Cache associativity: {}", level, associativity);
        log_info("L{} Cache type: {}", level, cache_type_name(type));
    };

    dumpCache(1, l1Cache);
    dumpCache(2, l2Cache);
    dumpCache(3, l3Cache);

#if CPU_X86 // TODO: AArch64
    for (std::size_t i = 0; i < k_amd64_extensions.size(); ++i) { // TODO: AArch64
        bool supported = instruction_set_supported(static_cast<iware::cpu::instruction_set_t>(i));
        log_info("{: <16} [{}]", k_amd64_extensions[i], supported ? 'X' : ' ');
    }
#endif
}

auto dump_memory_info() -> void {
    const double kGiB = std::pow(1024.0, 3.0);
    const auto mem = iware::system::memory();
    log_info("RAM physical total: {:.04F} GiB", static_cast<double>(mem.physical_total) / kGiB);
    log_info("RAM physical available: {:.04F} GiB", static_cast<double>(mem.physical_available) / kGiB);
    log_info("RAM virtual total: {:.04F} GiB", static_cast<double>(mem.virtual_total) / kGiB);
    log_info("RAM virtual available: {:.04F} GiB", static_cast<double>(mem.virtual_available) / kGiB);
}

auto dump_loaded_dylibs() -> void {
#if PLATFORM_WINDOWS
    std::array<HMODULE, 8192 << 2> hMods {};
    HANDLE hProcess = GetCurrentProcess();
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hMods.data(), sizeof(hMods), &cbNeeded)) {
        for (UINT i = 0; i < cbNeeded / sizeof(HMODULE); ++i) {
            std::array<TCHAR, MAX_PATH> szModName {};
            if (GetModuleFileNameEx(hProcess, hMods[i], szModName.data(), sizeof(szModName) / sizeof(TCHAR))) {
                log_info("{}", szModName.data());
            }
        }
    }
#else
    dl_iterate_phdr(+[](const dl_phdr_info* info, std::size_t, void*) -> int {
        LOG_INFO("{}", info->dlpi_name);
        return 0;
    }, nullptr);
#endif
}

    static constinit GLFWwindow* s_window;
    static constinit GLFWmonitor* s_monitor;
    static constinit void* s_native_window;
    static constinit void* s_native_display;

    static auto proxy_resize_hook(GLFWwindow* window, int w, int h) -> void {
        passert(window != nullptr);
        void* user = glfwGetWindowUserPointer(window);
        passert(user != nullptr);
        static_cast<platform_subsystem*>(user)->resize_hook(); // inform kernel about resize
        log_info("Resizing window to {}x{}", w, h);
    }

    platform_subsystem::platform_subsystem() : subsystem{"platform"} {
        passert(s_window == nullptr);

        log_info("Initializing platform backend");

        print_sep();
        dump_os_info();
        print_sep();
        dump_cpu_info();
        print_sep();
        dump_memory_info();
        print_sep();
        dump_loaded_dylibs();
        print_sep();

        // check that media directory exists TODO make this better
        passert(std::filesystem::exists("media"));

        // init glfw
        passert(glfwInit() == GLFW_TRUE);

        // create window
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // window is hidden by default
        s_window = glfwCreateWindow(1280, 720, "LunamEngine", nullptr, nullptr);
        passert(s_window != nullptr);
    #if PLATFORM_WINDOWS
        s_native_window = reinterpret_cast<void*>(glfwGetWin32Window(s_window));
    #endif // TODO: OSX, Linux
        passert(s_native_window != nullptr);
        glfwSetWindowUserPointer(s_window, this);

        // setup hooks
        glfwSetFramebufferSizeCallback(s_window, &proxy_resize_hook);

        // query monitor and print some info
        auto printMonitorInfo = [](GLFWmonitor* mon) {
            if (const char* name = glfwGetMonitorName(mon); name) {
                log_info("Monitor name: {}", name);
            }
            if (const GLFWvidmode* mode = glfwGetVideoMode(mon); mode) {
                log_info("Monitor: {}x{}@{}Hz", mode->width, mode->height, mode->refreshRate);
            }
            int n;
            if (const GLFWvidmode* modes = glfwGetVideoModes(mon, &n); modes) {
                log_info("Available modes:");
                for (int i = 0; i < n; ++i) {
                    const GLFWvidmode& mode = modes[i];
                    log_info("    {}x{}@{}Hz", mode.width, mode.height, mode.refreshRate);
                }
            }
        };
        s_monitor = glfwGetPrimaryMonitor();
        if (s_monitor != nullptr) [[likely]] {
            printMonitorInfo(s_monitor);
        } else {
            log_warn("No primary monitor found");
        }
        int n;
        if (GLFWmonitor** mons = glfwGetMonitors(&n); mons) {
            for (int i = 0; i < n; ++i) {
                if (mons[i] != s_monitor) {
                    printMonitorInfo(mons[i]);
                }
            }
        }

        // set window icon
        int w, h;
        stbi_uc *pixels = stbi_load(k_window_icon_file, &w, &h, nullptr, STBI_rgb_alpha);
        passert(pixels != nullptr);
        const GLFWimage icon {
            .width = w,
            .height = h,
            .pixels = pixels
        };
        glfwSetWindowIcon(s_window, 1, &icon);
        stbi_image_free(pixels);
    }

    platform_subsystem::~platform_subsystem() {
        s_window = nullptr;
        s_monitor = nullptr;
        s_native_window = nullptr;
        s_native_display = nullptr;
        glfwDestroyWindow(s_window);
        glfwTerminate();
    }

    void platform_subsystem::on_prepare() {
        glfwShowWindow(s_window);
        glfwFocusWindow(s_window);
    }

    bool platform_subsystem::on_pre_tick() {
        glfwPollEvents();
        return glfwWindowShouldClose(s_window) == GLFW_FALSE;
    }

    auto platform_subsystem::get_glfw_window() -> GLFWwindow* {
        return s_window;
    }

    auto platform_subsystem::get_native_window() -> void* {
        return s_native_window;
    }

    auto platform_subsystem::get_native_display() -> void* {
        return s_native_display;
    }
}
