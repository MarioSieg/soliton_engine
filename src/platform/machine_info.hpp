// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <infoware/infoware.hpp>

#include "../core/core.hpp"

namespace lu::platform {
    extern auto kernel_variant_name(iware::system::kernel_t variant) noexcept -> eastl::string_view;
    extern auto cache_type_name(iware::cpu::cache_type_t cache_type) noexcept -> eastl::string_view;
    extern auto architecture_name(iware::cpu::architecture_t architecture) noexcept -> eastl::string_view;
    extern auto endianness_name(iware::cpu::endianness_t endianness) noexcept -> eastl::string_view;
    extern auto print_os_info() -> void;
    extern auto print_cpu_info() -> void;
    extern auto print_memory_info() -> void;
    extern auto print_loaded_shared_modules() -> void;
    extern auto print_full_machine_info() -> void;

#if CPU_X86
    [[nodiscard]] extern auto is_avx_supported() noexcept -> bool;
    [[nodiscard]] extern auto is_avx2_supported() noexcept -> bool;
#endif
}
