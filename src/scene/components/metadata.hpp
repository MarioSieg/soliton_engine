// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../base.hpp"

namespace lu::com {
    struct entity_flags final {
        enum $ : std::uint32_t {
            none = 0,
            hidden = 1 << 0,
            transient = 1 << 1,
            static_object = 1 << 2
        };
    };

    struct metadata final {
        std::underlying_type_t<entity_flags::$> flags = entity_flags::none;
    };
}
