// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "subsystem.hpp"

template <typename T, typename... Ar>
concept is_subsystem = std::conjunction_v<std::is_base_of<subsystem, T>, std::is_constructible<T, Ar...>>;

class kernel final : public no_copy, public no_move {
public:
    kernel();
    ~kernel();

    template <typename T, typename... Ar> requires is_subsystem<T, Ar...>
    auto install(Ar&&... args) -> std::shared_ptr<T> {
        log_info("Installing subsystem: {}", typeid(T).name());
        auto subsystem = std::make_shared<T>(std::forward<Ar>(args)...);
        subsystem->resize_hook = [this] { this->resize(); };
        m_subsystems.emplace_back(subsystem);
        return subsystem;
    }

    [[nodiscard]] static auto get_delta_time() noexcept -> double;
    static auto request_exit() noexcept -> void;

    HOTPROC auto run() -> void;
    auto resize() -> void;

    [[nodiscard]] auto get_subsystems() const noexcept -> std::span<const std::shared_ptr<subsystem>> { return m_subsystems; }
    [[nodiscard]] auto get_boot_stamp() const noexcept -> std::chrono::high_resolution_clock::time_point { return boot_stamp; }

private:
    [[nodiscard]] HOTPROC auto tick() const -> bool;
    const std::chrono::high_resolution_clock::time_point boot_stamp = std::chrono::high_resolution_clock::now();
    std::vector<std::shared_ptr<subsystem>> m_subsystems {};
};
