// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"
#include "texture.hpp"

namespace graphics {
    // Precompute the BRDF (bidirectional reflectance distribution function) LUT
    extern auto generate_brdf_lut() -> std::unique_ptr<texture>;

    // Precompute the irradiance cube map
    [[nodiscard]] extern auto generate_irradiance_cube(
        const texture& env_map
    ) -> std::unique_ptr<texture>;

    // Precompute the prefiltered environment map
    [[nodiscard]] extern auto generate_prefiltered_env_map(
        const texture& env_map
    ) -> std::unique_ptr<texture>;
}
