// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "shared_buffers.hpp"
#include "vulkancore/context.hpp"

namespace lu::graphics {
    shared_buffers::shared_buffers() {
        vkb::descriptor_factory factory {vkb::ctx().descriptor_factory_begin()};
        vk::DescriptorBufferInfo info {
            .buffer = per_frame_ubo.get_buffer(),
            .offset = 0,
            .range = sizeof(glsl::perFrameData)
        };
        constexpr auto stages {
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
        };
        factory.bind_buffers(
            0,
            1,
            &info,
            vk::DescriptorType::eUniformBufferDynamic,
            static_cast<vk::ShaderStageFlagBits>(static_cast<std::underlying_type_t<vk::ShaderStageFlagBits>>(stages))
        );
        passert(factory.build(m_set, m_layout));
    }

    shared_buffers::~shared_buffers() {
        vkb::vkdvc().destroyDescriptorSetLayout(m_layout, vkb::get_alloc());
    }

    auto shared_buffers::get() -> shared_buffers& {
        static shared_buffers instance;
        return instance;
    }
}