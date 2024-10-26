// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "machine_info.hpp"

#if PLATFORM_WINDOWS
#   include <Windows.h>
#   include <Lmcons.h>
#   include <processthreadsapi.h>
#   include <cstdlib>
#   include <fcntl.h>
#   include <io.h>
#   include <psapi.h>
#elif PLATFORM_LINUX
#   include <link.h>
#elif PLATFORM_OSX
#   include <mach-o/dyld.h>
#endif

namespace lu::platform {
    auto kernel_variant_name(const iware::system::kernel_t variant) noexcept -> eastl::string_view {
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

    auto cache_type_name(const iware::cpu::cache_type_t cache_type) noexcept -> eastl::string_view {
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

    auto architecture_name(const iware::cpu::architecture_t architecture) noexcept -> eastl::string_view {
        switch (architecture) {
            case iware::cpu::architecture_t::x64:
                return "x86-64";
            case iware::cpu::architecture_t::arm:
                return "arm64";
            case iware::cpu::architecture_t::itanium:
                return "Itanium"; // not supported by lunam
            case iware::cpu::architecture_t::x86:
                return "x86"; // not supported by lunam
            default:
                return "unknown";
        }
    }

    auto endianness_name(const iware::cpu::endianness_t endianness) noexcept -> eastl::string_view {
        switch (endianness) {
            case iware::cpu::endianness_t::little:
                return "little-endian";
            case iware::cpu::endianness_t::big:
                return "big-endian";
            default:
                return "unknown";
        }
    }

#if CPU_X86
    static constexpr eastl::array<std::string_view, 38> k_amd64_extensions = {
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

#if CPU_X86
    auto is_avx_supported() noexcept -> bool {
        // Should return true for AMD Bulldozer, Intel "Sandy Bridge", and Intel "Ivy Bridge" or later processors
        // with OS support for AVX (Windows 7 Service Pack 1, Windows Server 2008 R2 Service Pack 1, Windows 8, Windows Server 2012)
        // See http://msdn.microsoft.com/en-us/library/hskdteyh.aspx
        int info[4] = {-1};
        #if (defined(__clang__) || defined(__GNUC__)) && defined(__cpuid)
            __cpuid(0, info[0], info[1], info[2], info[3]);
        #else
            __cpuid(info, 0 );
        #endif
        if (info[0] < 1 ) return false;
        #if (defined(__clang__) || defined(__GNUC__)) && defined(__cpuid)
            __cpuid(1, info[0], info[1], info[2], info[3]);
        #else
            __cpuid(info, 1 );
        #endif
        return (info[2] & 0x18080001) == 0x18080001;
    }

    auto is_avx2_supported() noexcept -> bool {
        // Should return true for AMD "Excavator", Intel "Haswell" or later processors
        // with OS support for AVX (Windows 7 Service Pack 1, Windows Server 2008 R2 Service Pack 1, Windows 8, Windows Server 2012)
        // See http://msdn.microsoft.com/en-us/library/hskdteyh.aspx
        int info[4] = {-1};
        #if (defined(__clang__) || defined(__GNUC__)) && defined(__cpuid)
            __cpuid(0, info[0], info[1], info[2], info[3]);
        #else
            __cpuid(info, 0);
        #endif
            if (info[0] < 7) return false;
        #if (defined(__clang__) || defined(__GNUC__)) && defined(__cpuid)
            __cpuid(1, info[0], info[1], info[2], info[3]);
        #else
            __cpuid(info, 1);
        #endif
        // Check for F16C, FMA3, AVX, OSXSAVE, SSSE4.1, and SSE3
        if ((info[2] & 0x38081001) != 0x38081001) return false;
        #if defined(__clang__) || defined(__GNUC__)
            __cpuid_count(7, 0, info[0], info[1], info[2], info[3]);
        #else
            __cpuidex(info, 7, 0);
        #endif
        return (info[1] & 0x20) == 0x20;
    }
#endif

    auto print_os_info() -> void {
        const static auto os_info = iware::system::OS_info();
        log_info("OS: {}, v.{}.{}.{}.{}", os_info.full_name, os_info.major, os_info.minor, os_info.patch, os_info.build_number);
        #if PLATFORM_WINDOWS
            TCHAR username[UNLEN+1];
            DWORD username_len = UNLEN+1;
            GetUserName(username, &username_len);
            log_info("Username: {}", username);
        #endif
    }

    auto print_cpu_info() -> void {
        const std::string name = iware::cpu::model_name();
        const std::string vendor = iware::cpu::vendor();
        const std::string vendor_id = iware::cpu::vendor_id();
        const auto [logical, physical, packages] = iware::cpu::quantities();
        const eastl::string_view arch = architecture_name(iware::cpu::architecture());
        const double base_freq_ghz = static_cast<double>(iware::cpu::frequency()) / std::pow(1024.0, 3);
        const eastl::string_view endianness = endianness_name(iware::cpu::endianness());
        const iware::cpu::cache_t cache_l1 = iware::cpu::cache(1);
        const iware::cpu::cache_t cache_l2 = iware::cpu::cache(2);
        const iware::cpu::cache_t cache_l3 = iware::cpu::cache(3);

        log_info("CPU(s): {} X {}", packages, name);
        log_info("CPU Architecture: {}", arch);
        log_info("CPU Base frequency: {:.02F} ghz", base_freq_ghz);
        log_info("CPU Endianness: {}",  endianness);
        log_info("CPU Logical cores: {}", logical);
        log_info("CPU Physical cores: {}", physical);
        log_info("CPU Sockets: {}", packages);
        log_info("CPU Vendor: {}", vendor);
        log_info("CPU Vendor ID: {}", vendor_id);

        const std::size_t hwc = std::thread::hardware_concurrency();
        log_info("Hardware concurrency: {}, machine class: {}", hwc, hwc >= 12 ? "EXCELLENT" : hwc >= 8 ? "GOOD" : "BAD");

        static constexpr auto dump_cache_info = [](unsigned level, const iware::cpu::cache_t& cache) {
            const auto [size, line_size, associativity, type] {cache};
            log_info("L{} Cache size: {} KiB", level, static_cast<double>(size) / 1024.0);
            log_info("L{} Cache line size: {} B", level, line_size);
            log_info("L{} Cache associativity: {}", level, associativity);
            log_info("L{} Cache type: {}", level, cache_type_name(type));
        };

        dump_cache_info(1, cache_l1);
        dump_cache_info(2, cache_l2);
        dump_cache_info(3, cache_l3);

        #if CPU_X86
            for (std::size_t i = 0; i < k_amd64_extensions.size(); ++i) {
                bool supported = instruction_set_supported(static_cast<iware::cpu::instruction_set_t>(i));
                // Infoware detects AVX on some CPUs which do not actually support it
                // probably a bug in the detection code or OSXSAVE is not tested/enabled
                // so AVX and AVX2 also also validated with cpuid manually to provide correct results
                switch (i) {
                    case static_cast<std::size_t>(iware::cpu::instruction_set_t::avx):
                        supported &= is_avx_supported();
                    break;
                    case static_cast<std::size_t>(iware::cpu::instruction_set_t::avx2):
                        supported &= is_avx2_supported();
                    break;
                }
                log_info("{: <16} [{}]", k_amd64_extensions[i], supported ? 'X' : ' ');
            }
        #endif
    }

    auto print_memory_info() -> void {
        constexpr auto kGiB = static_cast<double>(1ull<<30);
        const iware::system::memory_t memory_info = iware::system::memory();
        log_info("RAM physical total: {:.04f} GiB", static_cast<double>(memory_info.physical_total) / kGiB);
        log_info("RAM physical available: {:.04f} GiB", static_cast<double>(memory_info.physical_available) / kGiB);
        log_info("RAM virtual total: {:.04f} GiB", static_cast<double>(memory_info.virtual_total) / kGiB);
        log_info("RAM virtual available: {:.04f} GiB", static_cast<double>(memory_info.virtual_available) / kGiB);
    }

    auto print_loaded_shared_modules() -> void {
        #if PLATFORM_WINDOWS
            eastl::array<HMODULE, 8192<<2> hMods {};
            HANDLE hProcess = GetCurrentProcess();
            DWORD cbNeeded;
            if (EnumProcessModules(hProcess, hMods.data(), sizeof(hMods), &cbNeeded)) {
                for (UINT i = 0; i < cbNeeded / sizeof(HMODULE); ++i) {
                    eastl::array<TCHAR, MAX_PATH> szModName {};
                    if (GetModuleFileNameEx(hProcess, hMods[i], szModName.data(), sizeof(szModName) / sizeof(TCHAR))) {
                        log_info("Loaded module #{} '{}'", i, szModName.data());
                    }
                }
            }
        #elif PLATFORM_LINUX
            dl_iterate_phdr(+[](dl_phdr_info* info, std::size_t, void*) -> int {
                if (info->dlpi_name && *info->dlpi_name) log_info("Loaded module #{} '{}'", i, info->dlpi_name);
                return 0;
            }, nullptr);
        #elif PLATFORM_OSX
            const std::uint32_t num_images = _dyld_image_count();
            for (std::uint32_t i = 0; i < num_images; ++i) {
                const char* const name = _dyld_get_image_name(i);
                if (name && *name) log_info("Loaded module #{} '{}'", i, name);
            }
        #endif
    }

    auto print_full_machine_info() -> void {
        print_sep();
        print_os_info();
        print_sep();
        print_cpu_info();
        print_sep();
        print_memory_info();
        print_sep();
        print_loaded_shared_modules();
        print_sep();
    }
}
