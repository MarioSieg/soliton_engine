// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../vulkancore/prelude.hpp"
#include "../texture.hpp"

namespace lu::graphics {
    [[nodiscard]] extern auto raw_parse_texture(
        eastl::span<const std::byte> buf,
        const eastl::function<auto(const texture_descriptor& info, const texture_data_supplier& data) -> void>& callback
    ) -> bool;
}
