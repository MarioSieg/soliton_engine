// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <limits>

namespace lu {
    template <typename Scalar, const std::size_t Size> requires std::is_arithmetic_v<Scalar>
    struct fixed_ring_scalar_buffer final {
        auto submit(Scalar value) noexcept -> void;
        [[nodiscard]] auto offset() const noexcept -> std::size_t;
        [[nodiscard]] auto data() const noexcept -> const eastl::array<Scalar, Size>&;
        [[nodiscard]] auto min() const noexcept -> Scalar;
        [[nodiscard]] auto max() const noexcept -> Scalar;
        [[nodiscard]] auto avg() const noexcept -> Scalar;

    private:
        eastl::array<Scalar, Size> m_buf {};
        std::size_t m_offs {};
        Scalar m_min {};
        Scalar m_max {};
        Scalar m_avg {};
    };

    template <typename Scalar, const std::size_t Size> requires std::is_arithmetic_v<Scalar>
    inline auto fixed_ring_scalar_buffer<Scalar, Size>::data() const noexcept -> const eastl::array<Scalar, Size>& {
        return this->m_buf;
    }

    template <typename Scalar, const std::size_t Size> requires std::is_arithmetic_v<Scalar>
    inline auto fixed_ring_scalar_buffer<Scalar, Size>::min() const noexcept -> Scalar {
        return this->m_min;
    }

    template <typename Scalar, const std::size_t Size> requires std::is_arithmetic_v<Scalar>
    inline auto fixed_ring_scalar_buffer<Scalar, Size>::max() const noexcept -> Scalar {
        return this->m_max;
    }

    template <typename Scalar, const std::size_t Size> requires std::is_arithmetic_v<Scalar>
    inline auto fixed_ring_scalar_buffer<Scalar, Size>::avg() const noexcept -> Scalar {
        return this->m_avg;
    }

    template <typename Scalar, const std::size_t Size> requires std::is_arithmetic_v<Scalar>
    inline auto fixed_ring_scalar_buffer<Scalar, Size>::offset() const noexcept -> std::size_t {
        return this->m_offs;
    }

    template <typename Scalar, const std::size_t Size> requires std::is_arithmetic_v<Scalar>
    auto fixed_ring_scalar_buffer<Scalar, Size>::submit(const Scalar value) noexcept -> void {
        this->m_buf[this->m_offs] = value;
        this->m_offs = (this->m_offs + 1) % Size;
        Scalar min = eastl::numeric_limits<Scalar>::max();
        Scalar max = eastl::numeric_limits<Scalar>::min();
        Scalar avg {};
        for (const Scalar* __restrict__ i { this->m_buf.begin() }, *const __restrict__ e = i + this->m_buf.size(); i < e; ++i)
        {
            min = std::min(min, *i);
            max = std::max(max, *i);
            avg += *i;
        }
        this->m_min = min;
        this->m_max = max;
        this->m_avg = avg / static_cast<Scalar>(Size);
    }
}