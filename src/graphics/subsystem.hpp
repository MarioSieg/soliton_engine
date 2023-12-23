// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/subsystem.hpp"

namespace graphics {
    class graphics_subsystem final : public subsystem {
    public:
        graphics_subsystem();
        ~graphics_subsystem() override;

        auto on_pre_tick() -> bool override;
        auto on_post_tick() -> void override;
        auto on_resize() -> void override;

    private:
        std::uint32_t m_reset_flags = 0;
    };
}
