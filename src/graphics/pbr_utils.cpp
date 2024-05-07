// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "pbr_utils.hpp"
#include "vulkancore/context.hpp"

namespace graphics {
    auto generate_brdf_lut() -> std::unique_ptr<texture> {
        const vk::Device dvc = vkb::vkdvc();

        std::unique_ptr<texture> result = nullptr;

        return result;
    }
}
