// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <type_traits>
#include <memory>
#include <optional>

#include "utils.hpp"

namespace soliton {
    /**
     * Container for lazy initialization.
     * Same as Lazy<T> but heap allocated and fully moveable,
     * even if the underlying object is not.
     * Construct using the .init(args) method.
     * Object is destroyed in the destructor like normally.
     * @tparam T The type. Must be conforming to Option<T>. (C++ 17 destructible requirements N4828)
     */
    template <typename T>
    struct lazy_flex final {
        constexpr lazy_flex() noexcept = default;
        constexpr lazy_flex(decltype(nullptr)) noexcept;
        constexpr lazy_flex(const lazy_flex&) noexcept(std::is_nothrow_copy_constructible_v<std::remove_cv_t<T>>) = default;
        constexpr lazy_flex(lazy_flex&&) noexcept = default;
        constexpr auto operator =(const lazy_flex&) noexcept(std::is_nothrow_copy_assignable_v<std::remove_cv_t<T>>) -> lazy_flex& = default;
        constexpr auto operator =(lazy_flex&&) noexcept-> lazy_flex& = default;
        ~lazy_flex() = default;

        template <typename... Args> requires std::is_constructible_v<T, Args...>
        auto init(Args&&... args) -> void;
        [[nodiscard]] auto is_init() const noexcept -> bool;
        auto swap(lazy_flex<T>& other) -> void;
        auto reset(eastl::unique_ptr<T>&& val) noexcept -> void;
        auto reset(decltype(nullptr)) noexcept -> void;
        auto reset() noexcept -> void;
        auto destruct() noexcept -> void;
        operator bool() const noexcept;
        auto operator *() const noexcept -> const T&;
        auto operator *() noexcept -> T&;
        auto operator ->() const noexcept -> const T*;
        auto operator ->() noexcept -> T*;
        auto operator =(eastl::unique_ptr<T>&& other) noexcept -> lazy_flex<T>&;
        auto operator =(decltype(nullptr)) noexcept -> lazy_flex<T>&;

    private:
        eastl::unique_ptr<T> m_dat {nullptr };
    };

    template<typename T>
    constexpr lazy_flex<T>::lazy_flex(decltype(nullptr)) noexcept { }

    template <typename T>
    template <typename... Args> requires std::is_constructible_v<T, Args...>
    inline auto lazy_flex<T>::init(Args&&... args) -> void {
        this->m_dat = MakeUnique<T>(std::forward<Args>(args)...);
    }

    template <typename T>
    inline auto lazy_flex<T>::is_init() const noexcept -> bool {
        return this->m_dat.operator bool();
    }

    template <typename T>
    inline lazy_flex<T>::operator bool() const noexcept {
        return this->is_init();
    }

    template <typename T>
    inline auto lazy_flex<T>::swap(lazy_flex<T>& other) -> void {
        std::swap(this->m_dat, other.m_dat);
    }

    template <typename T>
    inline auto lazy_flex<T>::operator *() const noexcept -> const T& {
        panic_assert(this->is_init());
        return *this->m_dat;
    }

    template <typename T>
    inline auto lazy_flex<T>::operator *() noexcept -> T& {
        panic_assert(this->is_init());
        return *this->m_dat;
    }

    template <typename T>
    inline auto lazy_flex<T>::reset(eastl::unique_ptr<T>&& val) noexcept -> void {
        this->m_dat = std::move(val);
    }

    template <typename T>
    inline auto lazy_flex<T>::reset(decltype(nullptr)) noexcept -> void {
        this->reset();
    }

    template <typename T>
    inline auto lazy_flex<T>::reset() noexcept -> void {
        this->m_dat.reset();
    }

    template <typename T>
    inline auto lazy_flex<T>::destruct() noexcept -> void {
        this->reset();
    }

    template <typename T>
    inline auto lazy_flex<T>::operator -> () const noexcept -> const T* {
        return &**this;
    }

    template <typename T>
    inline auto lazy_flex<T>::operator -> () noexcept -> T* {
        return &**this;
    }

    template <typename T>
    inline auto lazy_flex<T>::operator = (eastl::unique_ptr<T>&& other) noexcept -> lazy_flex<T>& {
        this->reset(std::move(other));
        return *this;
    }

    template <typename T>
    inline auto lazy_flex<T>::operator = (decltype(nullptr)) noexcept -> lazy_flex<T>& {
        this->reset();
        return *this;
    }
}
