// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/core.hpp"

namespace lu {
    class scene;

    [[nodiscard]] auto import_from_file(const eastl::string& path, std::uint32_t load_flags) -> eastl::unique_ptr<scene>;
}
