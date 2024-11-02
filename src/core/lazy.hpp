// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <type_traits>
#include <memory>
#include <optional>

#include "utils.hpp"

namespace lu {
    /**
     * Container for lazy initialization.
     * Construct using the .New(args) method.
     * Object is destroyed in the destructor like normally.
     * @tparam T The type. Must be conforming to Option<T>. (C++ 17 destructible requirements N4828)
     */
    template <typename T>
    struct lazy final {
        static_assert(!std::is_same_v<T, std::nullopt_t> && !std::is_same_v<T, std::in_place_t>, "T in Lazy<T> must be a type other than nullopt_t or in_place_t (N4828 [optional.optional]/3).");
        static_assert(std::is_object_v<T> && std::is_destructible_v<T> && !std::is_array_v<T>, "T in Lazy<T> must meet the C++17 destructible requirements (N4828 [optional.optional]/3).");

        template <typename T2>
        using allow_cvt = std::bool_constant<std::conjunction_v<std::negation<std::is_same<std::remove_cvref_t<T2>, lazy>>,
        std::negation<std::is_same<std::remove_cvref_t<T2>, std::in_place_t>>, std::is_constructible<T, T2>>>;

        template <typename T2>
        struct allow_unwrap : std::bool_constant<!std::disjunction_v<std::is_same<T, T2>, std::is_constructible<T, eastl::optional<T2>&>,
        std::is_constructible<T, const lazy<T2>&>,
        std::is_constructible<T, const lazy<T2>>, std::is_constructible<T, lazy<T2>>,
        std::is_convertible<lazy<T2>&, T>, std::is_convertible<const lazy<T2>&, T>,
        std::is_convertible<const lazy<T2>, T>, std::is_convertible<lazy<T2>, T>>> { };

        constexpr lazy() noexcept = default;
        constexpr lazy(decltype(nullptr)) noexcept;
        constexpr lazy(const lazy&) noexcept(std::is_nothrow_copy_constructible_v<std::remove_cv_t<T>>) = default;
        constexpr lazy(lazy&&) noexcept(std::is_nothrow_move_constructible_v<std::remove_cv_t<T>>) = default;
        constexpr auto operator =(const lazy&) noexcept(std::is_nothrow_copy_assignable_v<std::remove_cv_t<T>>) -> lazy& = default;
        constexpr auto operator =(lazy&&) noexcept(std::is_nothrow_move_assignable_v<std::remove_cv_t<T>>) -> lazy& = default;
        ~lazy() = default;

        template <typename... Args> requires std::is_constructible_v<std::remove_cv_t<T>, Args...>
        constexpr auto init(Args&&... args) noexcept(std::is_nothrow_constructible_v<std::remove_cv_t<T>, Args...>) -> void;
        constexpr auto from(T&& instance) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_copy_constructible_v<T>) -> void;
        [[nodiscard]] constexpr auto is_init() const noexcept -> bool;
        constexpr auto swap(lazy<T>& other) -> void;
        constexpr auto reset(T&& val) noexcept(std::is_nothrow_constructible_v<std::remove_cv_t<T>, T&&>) -> void;
        constexpr auto reset(decltype(nullptr)) noexcept -> void;
        constexpr auto reset() noexcept -> void;
        constexpr auto destruct() noexcept -> void;
        constexpr operator bool() const noexcept;
        constexpr auto operator *() const noexcept -> const T&;
        constexpr auto operator *() noexcept -> T&;
        constexpr auto operator ->() const noexcept -> const T*;
        constexpr auto operator ->() noexcept -> T*;
        constexpr auto operator =(T&& other) noexcept(std::is_nothrow_copy_assignable_v<T>) -> lazy<T>&;
        constexpr auto operator =(decltype(nullptr)) noexcept -> lazy<T>&;

    private:
        eastl::optional<T> m_dat {std::nullopt };
    };

    template <typename T>
    constexpr lazy<T>::lazy(decltype(nullptr)) noexcept { }

    template <typename T>
    template <typename... Args> requires std::is_constructible_v<std::remove_cv_t<T>, Args...>
    constexpr auto lazy<T>::init(Args&&... args) noexcept(std::is_nothrow_constructible_v<std::remove_cv_t<T>, Args...>) -> void {
        this->m_dat = decltype(this->m_dat) {std::in_place, std::forward<Args>(args)... };
    }

    template <typename T>
    constexpr auto lazy<T>::from(T&& instance) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_copy_constructible_v<T>) -> void {
        this->m_dat = decltype(this->m_dat) {std::in_place, std::forward<T>(instance) };
    }

    template <typename T>
    constexpr auto lazy<T>::is_init() const noexcept -> bool {
        return this->m_dat.has_value();
    }

    template <typename T>
    constexpr lazy<T>::operator bool() const noexcept {
        return this->is_init();
    }

    template <typename T>
    constexpr auto lazy<T>::swap(lazy<T>& other) -> void {
        std::swap(this->m_dat, other.m_dat);
    }

    template <typename T>
    constexpr auto lazy<T>::operator *() const noexcept -> const T& {
        panic_assert(this->is_init());
        return *this->m_dat;
    }

    template <typename T>
    constexpr auto lazy<T>::operator *() noexcept -> T& {
        panic_assert(this->is_init());
        return *this->m_dat;
    }

    template <typename T>
    constexpr auto lazy<T>::reset(T&& val) noexcept(std::is_nothrow_constructible_v<std::remove_cv_t<T>, T&&>) -> void {
        this->m_dat = {std::forward<T>(val) };
    }

    template <typename T>
    constexpr auto lazy<T>::reset(decltype(nullptr)) noexcept -> void {
        this->reset();
    }

    template <typename T>
    constexpr auto lazy<T>::reset() noexcept -> void {
        this->m_dat.reset();
    }

    template <typename T>
    constexpr auto lazy<T>::destruct() noexcept -> void {
        this->reset();
    }

    template <typename T>
    constexpr auto lazy<T>::operator -> () const noexcept -> const T* {
        return &**this;
    }

    template <typename T>
    constexpr auto lazy<T>::operator -> () noexcept -> T* {
        return &**this;
    }

    template <typename T>
    constexpr auto lazy<T>::operator = (T&& other) noexcept(std::is_nothrow_copy_assignable_v<T>) -> lazy<T>& {
        this->reset(std::forward<T>(other));
        return *this;
    }

    template <typename T>
    constexpr auto lazy<T>::operator = (decltype(nullptr)) noexcept -> lazy<T>& {
        this->reset();
        return *this;
    }
}
