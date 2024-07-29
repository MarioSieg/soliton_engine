// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "subsystem.hpp"

namespace lu {
    template <typename T, typename... Ar>
    concept is_subsystem = std::conjunction_v<std::is_base_of<subsystem, T>, std::is_constructible<T, Ar...>>;

    class kernel final : public no_copy, public no_move {
    public:
        kernel(int argc, const char** argv, const char** $environ);
        ~kernel();

        template <typename T, typename... Ar> requires is_subsystem<T, Ar...>
        auto install(Ar&&... args) -> std::shared_ptr<T> {
            auto subsystem = std::make_shared<T>(std::forward<Ar>(args)...);
            subsystem->resize_hook = [this] { this->resize(); };
            m_subsystems.emplace_back(subsystem);
            return subsystem;
        }

        [[nodiscard]] static auto get() noexcept -> kernel&;

        [[nodiscard]] auto get_delta_time() noexcept -> double;
        [[nodiscard]] auto get_time() noexcept -> double;
        auto request_exit() noexcept -> void;
        auto on_new_scene_start(scene& scene) -> void;

        HOTPROC auto run() -> void;
        auto resize() -> void;

        [[nodiscard]] auto get_subsystems() const noexcept -> eastl::span<const std::shared_ptr<subsystem>> { return m_subsystems; }
        [[nodiscard]] auto get_boot_stamp() const noexcept -> std::chrono::high_resolution_clock::time_point { return m_boot_stamp; }

        static inline const eastl::string config_dir = "config/";
        static inline const eastl::string log_dir = "log";

    private:
        [[nodiscard]] HOTPROC auto tick() -> bool;
        const std::chrono::high_resolution_clock::time_point m_boot_stamp = std::chrono::high_resolution_clock::now();
        eastl::vector<std::shared_ptr<subsystem>> m_subsystems {};
        std::uint64_t m_frame = 0;
    };
}
