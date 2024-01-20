// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <filesystem>

#include "../core/core.hpp"

#include "../dense/unordered_dense.h"
#include <bgfx/bgfx.h>

template <typename T> requires (sizeof(T) == sizeof(std::uint16_t))
    struct handle final { // Destroys BGFX when out of scope
    T value = BGFX_INVALID_HANDLE;

    explicit constexpr handle() noexcept = default;
    explicit constexpr handle(T value) noexcept : value{value} {}
    handle(const handle&) = delete;
    handle(handle&& other) noexcept : value{other.value} { other.value = BGFX_INVALID_HANDLE; }
    auto operator = (const handle&) -> handle& = delete;
    auto operator = (handle&& other) noexcept -> handle& {
        value = other.value;
        other.value = BGFX_INVALID_HANDLE;
        return *this;
    }
    [[nodiscard]] constexpr auto operator * () const noexcept -> T { return value; }
    operator bool() const noexcept { return bgfx::isValid(value); }

    ~handle() noexcept {
        if (bgfx::isValid(value)) {
            bgfx::destroy(value);
        }
    }
};

[[nodiscard]] extern auto load_shader(const std::string& path) -> bgfx::ShaderHandle;
extern auto load_shader_registry(
    std::filesystem::path&& root,
    ankerl::unordered_dense::map<std::string, handle<bgfx::ProgramHandle>>& registry
) -> void;
