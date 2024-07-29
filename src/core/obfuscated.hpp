// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "core.hpp"

namespace lu {
#ifndef LU_FORCEINLINE
#define LU_FORCEINLINE __attribute__((always_inline)) inline
#endif

    //Inline 32-bit random number generator. Not thread-safe as it stores state.
    LU_FORCEINLINE auto sec_rand() noexcept -> std::uint32_t {
        static constinit std::uint32_t next = 0xcb536e6a;
        next = next*214013 + 2531011;
        return next;
    }

    // This class obfuscates the contained data in memory by XORing it with a magic number
    // and the address of the data itself. Avoid bitwise copying of this type!
    // The contained type in sysObfuscated does not need to be a POD type, but note:
    // 1. External code accessing the object through its 'this' pointer will get obfuscated data, likely causing a crash.
    // 2. The copy constructor and assignment operator may be called in non-standard ways, potentially with a misaligned object.
    //
    // Template arguments:
    // T: The type to be obfuscated (must be a multiple of 32 bits).
    // Mutate: If true, the class size doubles, and the memory layout changes with each read, making it harder to detect data changes with breakpoints.
    // This has a performance impact compared to non-mutated types.
    template <typename T, bool Mutate=true>
    class obfuscated final : public no_copy, public no_move {
        static_assert((sizeof(T) & 3) == 0, "obfuscated<T> requires T to be a multiple of 32 bits");

        LU_FORCEINLINE obfuscated() noexcept {
            init();
        }
        ~obfuscated() = default;

        LU_FORCEINLINE auto get() const noexcept -> T {
            const std::uint32_t xor_v = m_xor ^ (std::uint32_t)(size_t)this;
            std::array<std::uint32_t, sizeof(T)/sizeof(std::uint32_t)> ret;
            auto* src = const_cast<std::uint32_t*>(&m_data[0]);
            auto* const dest = ret.data();
            for (std::size_t i = 0; i < (sizeof(T)>>2); ++i) {
                if constexpr (Mutate) {
                    const std::uint32_t x = *src & m_mutate;
                    const std::uint32_t y = src[sizeof(T) >> 2] & (~m_mutate);
                    const std::uint32_t entropy_x = ((*src&(~m_mutate))<<16) | ((*src&(~m_mutate))>>16);
                    const std::uint32_t entropy_y = ((src[sizeof(T)>>2]&m_mutate)<<16) | ((src[sizeof(T)>>2]&m_mutate)>>16);
                    *src = (*src & m_mutate) | entropy_x;
                    src[sizeof(T)>>2] = (src[sizeof(T)>>2]&(~m_mutate))|entropy_y;
                    *dest++ = x|y;
                    ++src;
                }
                else
                {
                    *dest++ = *src++ ^ xor_v;
                }
            }
            if constexpr (Mutate) {
                const_cast<obfuscated<T, Mutate>*>(this)->set(std::bit_cast<T>(ret));
            }
            return std::bit_cast<T>(ret);
        }

        LU_FORCEINLINE auto set(const T& value) noexcept -> void {
            init();
            const std::uint32_t xor_v = m_xor ^ static_cast<std::uint32_t>(std::bit_cast<std::uintptr_t>(this));
            auto* src = reinterpret_cast<const std::uint32_t*>(&value);
            auto* const dest = m_data.data();
            for (std::size_t i = 0; i < (sizeof(T)>>2); ++i) {
                if constexpr (Mutate) {
                    const std::uint32_t a = *src & m_mutate;
                    const std::uint32_t b = *src & ~m_mutate;
                    ++src;
                    *dest = a;
                    dest[sizeof(T)>>2] = b;
                    ++dest;
                } else {
                    *dest++ = *src++ ^ xor_v;
                }
            }
        }

        LU_FORCEINLINE inline auto operator == (const obfuscated<T>& rhs) const noexcept { return get() == rhs.get(); }
        LU_FORCEINLINE inline auto operator != (const obfuscated<T>& rhs) const noexcept { return get() != rhs.get(); }
        LU_FORCEINLINE inline auto operator < (const obfuscated<T>& rhs) const noexcept { return get() < rhs.get(); }
        LU_FORCEINLINE inline auto operator > (const obfuscated<T>& rhs) const noexcept { return get() > rhs.get(); }
        LU_FORCEINLINE inline auto operator <= (const obfuscated<T>& rhs) const noexcept { return get() <= rhs.get(); }
        LU_FORCEINLINE inline auto operator >= (const obfuscated<T>& rhs) const noexcept { return get() >= rhs.get(); }

        LU_FORCEINLINE inline auto operator = (const T& rhs) noexcept -> obfuscated<T>& {
            set(rhs);
            return *this;
        }
        LU_FORCEINLINE inline operator T() const noexcept { return get(); }

    private:
        mutable std::array<std::uint32_t, (Mutate ? (sizeof(T)<<1) : sizeof(T)) / sizeof(std::uint32_t)> m_data {};
        mutable std::uint32_t m_xor {};
        mutable std::uint32_t m_mutate {};

        LU_FORCEINLINE auto init() -> void {
            m_xor = sec_rand();
            if constexpr (Mutate) {
                m_mutate = sec_rand();
            }
        }
    };

    // TODO:
    // Class keeps track of multiple copies of a variable, and if they go out of sync, such as by someone modifying the value in memory a callback is fired.
}
