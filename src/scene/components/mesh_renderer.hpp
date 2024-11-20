// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../base.hpp"

namespace soliton::graphics {
    class mesh;
    class texture;
    class material;
}

namespace soliton::com {
    struct render_flags final {
        enum $ : std::uint32_t {
            none = 0,
            skip_rendering = 1 << 0,
            skip_frustum_culling = 1 << 1,
        };
    };

    struct mesh_renderer final {
        eastl::fixed_vector<graphics::mesh*, 4> meshes {};
        eastl::fixed_vector<graphics::material*, 4> materials {};
        std::underlying_type_t<render_flags::$> flags = render_flags::none;
    };
}
