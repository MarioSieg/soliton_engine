// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <imgui.h>
#include "../vulkancore/context.hpp"
#include "../../core/core.hpp"

namespace soliton::imgui {
    class context : public no_copy, public no_move {
    public:
        context();
        ~context();

        auto begin_frame() -> void;
        auto end_frame() -> void;
        auto submit_imgui(vk::CommandBuffer cmd_buf) -> void;

    private:
        vk::DescriptorPool m_imgui_descriptor_pool {};
    };
}
