// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"
#include "vulkancore/buffer.hpp"

namespace lu::graphics {
    class shared_buffers final : public no_copy, public no_move {
    public:
        shared_buffers();
        ~shared_buffers();

        vkb::uniform_buffer<glsl::per_frame_data> per_frame_ubo {};

        [[nodiscard]] inline auto get_layout() const noexcept -> const vk::DescriptorSetLayout& { return m_layout; }
        [[nodiscard]] inline auto get_set() const noexcept -> const vk::DescriptorSet& { return m_set; }
        [[nodiscard]] static inline auto get() -> eastl::unique_ptr<shared_buffers>& {
            if (!m_instance) m_instance = eastl::make_unique<shared_buffers>();
            return m_instance;
        }

    private:
        static inline eastl::unique_ptr<shared_buffers> m_instance {};
        vk::DescriptorSetLayout m_layout {};
        vk::DescriptorSet m_set {};
    };
}