// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "shader_registry.hpp"

#include <filesystem>

namespace graphics {
    using namespace std::filesystem;

    auto shader_registry::compile_all() -> bool {
        m_shaders.clear();
        vkb::shader::init_shader_compiler();
        for (auto&& entry : recursive_directory_iterator{m_shader_dir}) {
            const auto& path = entry.path();
            const auto name = path.filename().string();
            log_info("Compiling shader: {} from {}", name, path.string());
            auto shader = vkb::shader::compile(path.string());
            if (!shader) {
                log_error("Failed to compile shader: {}", name);
                return false;
            }
            m_shaders[name] = std::move(shader);
        }
        vkb::shader::shutdown_shader_compiler();
        return true;
    }
}
