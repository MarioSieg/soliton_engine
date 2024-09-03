// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "core.hpp"

namespace lu {
    class splash_screen final : public no_copy, public no_move {
    public:
        explicit splash_screen(const char* logoPath);
        ~splash_screen();
        auto show() -> void;
    };
}
