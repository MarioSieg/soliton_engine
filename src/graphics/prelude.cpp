// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "prelude.hpp"

#include <filesystem>

namespace graphics {
    auto load_shader_program(const std::string& path) -> handle<bgfx::ProgramHandle> {
        passert(std::filesystem::exists(path));

    }
}
