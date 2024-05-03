// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "platform_subsystem.hpp"

#include <iostream>
#include <filesystem>

#include "../graphics/graphics_subsystem.hpp"

#include <GLFW/glfw3.h>
#if PLATFORM_WINDOWS // TODO: OSX, Linux
#include <Windows.h>
#include <Lmcons.h>
#include <processthreadsapi.h>
#include <cstdlib>
#include <fcntl.h>
#include <io.h>
#include <psapi.h>
#elif PLATFORM_LINUX
#include <link.h>
#endif
#include <mimalloc.h>
#include <infoware/infoware.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <nfd.hpp>
#include "../scripting/scripting_subsystem.hpp"

using scripting::scripting_subsystem;

namespace platform {
[[maybe_unused]]
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

#if CPU_X86
[[nodiscard]] inline auto is_avx_supported() noexcept -> bool {
    // Should return true for AMD Bulldozer, Intel "Sandy Bridge", and Intel "Ivy Bridge" or later processors
    // with OS support for AVX (Windows 7 Service Pack 1, Windows Server 2008 R2 Service Pack 1, Windows 8, Windows Server 2012)

    // See http://msdn.microsoft.com/en-us/library/hskdteyh.aspx
    int CPUInfo[4] = {-1};
#if (defined(__clang__) || defined(__GNUC__)) && defined(__cpuid)
    __cpuid(0, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
#else
    __cpuid( CPUInfo, 0 );
#endif

    if ( CPUInfo[0] < 1  )
        return false;

#if (defined(__clang__) || defined(__GNUC__)) && defined(__cpuid)
    __cpuid(1, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
#else
    __cpuid(CPUInfo, 1 );
#endif

    // We check for AVX, OSXSAVE, SSSE4.1, and SSE3
    return (CPUInfo[2] & 0x18080001) == 0x18080001;
}

[[nodiscard]] inline auto is_avx2_supported() noexcept -> bool {
    // Should return true for AMD "Excavator", Intel "Haswell" or later processors
    // with OS support for AVX (Windows 7 Service Pack 1, Windows Server 2008 R2 Service Pack 1, Windows 8, Windows Server 2012)

    // See http://msdn.microsoft.com/en-us/library/hskdteyh.aspx
    int CPUInfo[4] = {-1};
#if (defined(__clang__) || defined(__GNUC__)) && defined(__cpuid)
    __cpuid(0, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
#else
    __cpuid(CPUInfo, 0);
#endif

    if (CPUInfo[0] < 7)
        return false;

#if (defined(__clang__) || defined(__GNUC__)) && defined(__cpuid)
    __cpuid(1, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
#else
    __cpuid(CPUInfo, 1);
#endif

    // We check for F16C, FMA3, AVX, OSXSAVE, SSSE4.1, and SSE3
    if ((CPUInfo[2] & 0x38081001) != 0x38081001)
        return false;

#if defined(__clang__) || defined(__GNUC__)
    __cpuid_count(7, 0, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
#else
    __cpuidex(CPUInfo, 7, 0);
#endif

    return (CPUInfo[1] & 0x20) == 0x20;
}
#endif

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

    const std::size_t hwc = std::thread::hardware_concurrency();
    log_info("Hardware concurrency: {}, machine class: {}", hwc, hwc >= 12 ? "EXCELLENT" : hwc >= 8 ? "GOOD" : "BAD");

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
        // infoware detects AVX on some CPUs which do not actually support it
        // probably a bug in the windows detection code
        // so AVX and AVX2 also also validated with cpuid manually to provide correct results
        if (i == static_cast<std::size_t>(iware::cpu::instruction_set_t::avx)) {
            supported &= is_avx_supported();
        } else if (i == static_cast<std::size_t>(iware::cpu::instruction_set_t::avx2)) {
            supported &= is_avx2_supported();
        }
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
#elif PLATFORM_LINUX
    dl_iterate_phdr(+[](dl_phdr_info* info, std::size_t, void*) -> int {
        log_info("{}", info->dlpi_name);
        return 0;
    }, nullptr);
#endif
}

    static constinit GLFWwindow* s_window;

    static auto proxy_resize_hook(GLFWwindow* window, int w, int h) -> void {
        passert(window != nullptr);
        void* user = glfwGetWindowUserPointer(window);
        passert(user != nullptr);
        static_cast<platform_subsystem*>(user)->resize_hook(); // inform kernel about resize
        log_info("Resizing window to {}x{}", w, h);
    }

    static auto glfw_cursor_pos_callback(GLFWwindow* window, double x, double y) noexcept -> void {
        for (auto* const cb : platform_subsystem::s_cursor_pos_callbacks) {
            cb(window, x, y);
        }
    }
    static auto glfw_scroll_callback(GLFWwindow* window, double x, double y) noexcept -> void {
        for (auto* const cb : platform_subsystem::s_scroll_callbacks) {
            cb(window, x, y);
        }
    }
    static auto glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept -> void {
        for (auto* const cb : platform_subsystem::s_key_callbacks) {
            cb(window, key, scancode, action, mods);
        }
    }
    static auto glfw_char_callback(GLFWwindow* window, unsigned int codepoint) noexcept -> void {
        for (auto* const cb : platform_subsystem::s_char_callbacks) {
            cb(window, codepoint);
        }
    }
    static auto glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) noexcept -> void {
        for (auto* const cb : platform_subsystem::s_mouse_button_callbacks) {
            cb(window, button, action, mods);
        }
    }
    static auto glfw_cursor_enter_callback(GLFWwindow* window, int entered) noexcept -> void {
        for (auto* const cb : platform_subsystem::s_cursor_enter_callbacks) {
            cb(window, entered);
        }
    }
    static auto glfw_framebuffer_size_callback(GLFWwindow* window, int w, int h) noexcept -> void {
        for (auto* const cb : platform_subsystem::s_framebuffer_size_callbacks) {
            cb(window, w, h);
        }
    }

    platform_subsystem::platform_subsystem() : subsystem{"Platform"} {
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

        // init glfw
        glfwSetErrorCallback([](const int code, const char* desc) {
            log_error("Platform error: {} ({:#X})", desc, code);
        });
        static GLFWallocator allocator {};
        allocator.allocate = [](const std::size_t size, [[maybe_unused]] void* usr) -> void* {
            return mi_malloc(size);
        };
        allocator.deallocate = [](void* ptr, [[maybe_unused]] void* usr) -> void {
            mi_free(ptr);
        };
        allocator.reallocate = [](void* ptr, const std::size_t size, [[maybe_unused]] void* usr) -> void* {
            return mi_realloc(ptr, size);
        };
        glfwInitAllocator(&allocator);
        const bool is_glfw_online = glfwInit() == GLFW_TRUE;
        if (!is_glfw_online) [[unlikely]] {
            char const* desc;
            glfwGetError(&desc);
            if (desc) {
                log_error("Failed to initialize GLFW: {}", desc);
            }
            passert(is_glfw_online);
        }

        const int default_width = scripting_subsystem::cfg()["Window"]["defaultWidth"].cast<int>().valueOr(1280);
        const int default_height = scripting_subsystem::cfg()["Window"]["defaultHeight"].cast<int>().valueOr(720);
        const int min_width = scripting_subsystem::cfg()["Window"]["minWidth"].cast<int>().valueOr(640);
        const int min_height = scripting_subsystem::cfg()["Window"]["minHeight"].cast<int>().valueOr(480);

        // create window
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // window is hidden by default
        s_window = glfwCreateWindow(default_width, default_height, "LunamEngine", nullptr, nullptr);
        passert(s_window != nullptr);
        glfwSetWindowUserPointer(s_window, this);
        glfwSetWindowSizeLimits(s_window, min_width, min_height, GLFW_DONT_CARE, GLFW_DONT_CARE);

        // setup hooks
        glfwSetCursorPosCallback(s_window, &glfw_cursor_pos_callback);
        glfwSetScrollCallback(s_window, &glfw_scroll_callback);
        glfwSetKeyCallback(s_window, &glfw_key_callback);
        glfwSetCharCallback(s_window, &glfw_char_callback);
        glfwSetMouseButtonCallback(s_window, &glfw_mouse_button_callback);
        glfwSetCursorEnterCallback(s_window, &glfw_cursor_enter_callback);
        glfwSetFramebufferSizeCallback(s_window, &glfw_framebuffer_size_callback);

        // setup framebuffer resize hook
        s_framebuffer_size_callbacks.emplace_back(&proxy_resize_hook);

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
        GLFWmonitor* mon = glfwGetPrimaryMonitor();
        if (mon != nullptr) [[likely]] {
            printMonitorInfo(mon);
            if (const GLFWvidmode* mode = glfwGetVideoMode(mon); mode) {
                glfwSetWindowPos(s_window, std::max((mode->width>>1)-default_width, min_width),
                    std::max((mode->height>>1)-default_height, min_height));
            }
        } else {
            log_warn("No primary monitor found");
        }
        int n;
        if (GLFWmonitor** mons = glfwGetMonitors(&n); mons) {
            for (int i = 0; i < n; ++i) {
                if (mons[i] != mon) {
                    printMonitorInfo(mons[i]);
                }
            }
        }

        // set window icon
        if constexpr (!PLATFORM_OSX) { // Cocoa - regular windows do not have icons on macOS
            std::vector<std::uint8_t> pixel_buf {};
            const std::string k_window_icon_file = scripting_subsystem::cfg()["Window"]["icon"].cast<std::string>().valueOr("icon.png");
            assetmgr::load_asset_blob_or_panic(k_window_icon_file, pixel_buf);
            int w, h;
            stbi_uc *pixels = stbi_load_from_memory(pixel_buf.data(), static_cast<int>(pixel_buf.size()), &w, &h, nullptr, STBI_rgb_alpha);
            passert(pixels != nullptr);
            const GLFWimage icon {
                .width = w,
                .height = h,
                .pixels = pixels
            };
            glfwSetWindowIcon(s_window, 1, &icon);
            stbi_image_free(pixels);
        }
        NFD_Init();
    }

    platform_subsystem::~platform_subsystem() {
        NFD_Quit();
        s_window = nullptr;
        glfwDestroyWindow(s_window);
        glfwTerminate();
    }

    void platform_subsystem::on_prepare() {
        glfwShowWindow(s_window);
        glfwFocusWindow(s_window);
    }

    HOTPROC auto platform_subsystem::on_pre_tick() -> bool {
        glfwPollEvents();
        return glfwWindowShouldClose(s_window) == GLFW_FALSE;
    }

    auto platform_subsystem::get_glfw_window() -> GLFWwindow* {
        return s_window;
    }
}
