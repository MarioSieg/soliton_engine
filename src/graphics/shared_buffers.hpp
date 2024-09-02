// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"
#include "vulkancore/buffer.hpp"

namespace lu::graphics {
    class shared_buffers final : public no_copy, public no_move {
    public:
        shared_buffers();
        ~shared_buffers();

        vkb::uniform_buffer<glsl::perFrameData> per_frame_ubo {};

        [[nodiscard]] inline auto get_layout() const noexcept -> const vk::DescriptorSetLayout& { return m_layout; }
        [[nodiscard]] inline auto get_set() const noexcept -> const vk::DescriptorSet& { return m_set; }
        [[nodiscard]] static auto get() -> shared_buffers&;


    private:
        vk::DescriptorSetLayout m_layout {};
        vk::DescriptorSet m_set {};
    };
}