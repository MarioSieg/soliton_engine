// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "subsystem.hpp"

namespace soliton {
    template <typename T, typename... Ar>
    concept is_subsystem = std::conjunction_v<std::is_base_of<subsystem, T>, std::is_constructible<T, Ar...>>;

    class kernel final : public no_copy, public no_move {
    public:
        kernel(int argc, const char** argv, const char** $environ);
        ~kernel();

        template <typename T, typename... Ar> requires is_subsystem<T, Ar...>
        auto install(Ar&&... args) -> eastl::shared_ptr<T> {
            stopwatch clock {};
            auto subsystem = eastl::make_shared<T>(std::forward<Ar>(args)...);
            subsystem->resize_hook = [this] { this->resize(); };
            m_subsystems.emplace_back(subsystem);
            dynamic_cast<class subsystem*>(&*subsystem)->m_boot_time = clock.elapsed<eastl::chrono::nanoseconds>();
            return subsystem;
        }

        [[nodiscard]] static auto get() noexcept -> kernel&;

        [[nodiscard]] auto get_delta_time() noexcept -> double;
        [[nodiscard]] auto get_time() noexcept -> double;

        auto request_exit() noexcept -> void;
        auto start_scene(scene& scene) -> void;

        HOTPROC auto run() -> void;
        auto resize() -> void;

        [[nodiscard]] auto get_subsystems() const noexcept -> eastl::span<const eastl::shared_ptr<subsystem>> { return m_subsystems; }
        [[nodiscard]] auto get_boot_stamp() const noexcept -> eastl::chrono::high_resolution_clock::time_point { return m_boot_stamp; }

    private:
        [[nodiscard]] HOTPROC auto tick() -> bool;
        const eastl::chrono::high_resolution_clock::time_point m_boot_stamp = eastl::chrono::high_resolution_clock::now();
        eastl::vector<eastl::shared_ptr<subsystem>> m_subsystems {};
        std::uint64_t m_frame = 0;
    };
}
