// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "shader_registry.hpp"

#include <future>
#include <filesystem>

namespace graphics {

    shader_registry::shader_registry(std::string&& shader_dir) : m_shader_dir{std::move(shader_dir)} {
        if (!std::filesystem::exists(m_shader_dir)) {
            panic("Shader directory does not exist: {}", m_shader_dir);
        }
    }

    [[nodiscard]] auto shader_registry::get_shader(const std::string& name) const -> std::shared_ptr<vkb::shader> {
        if (!m_shaders.contains(name)) [[unlikely]] {
            panic("Shader not within registry: {}", name);
        }
        return m_shaders.at(name);
    }

    auto shader_registry::compile_all() -> bool {
        using namespace std::filesystem;
        m_shaders.clear();
        std::vector<std::future<std::pair<std::string, std::shared_ptr<vkb::shader>>>> futures {};
        for (auto&& entry : recursive_directory_iterator{m_shader_dir}) {
            const auto& path = entry.path();
            auto name = path.filename().string();
            auto shader = vkb::shader::compile(path.string());
            futures.emplace_back(std::async(std::launch::async, [](std::string&& name, std::string&& path) -> std::pair<std::string, std::shared_ptr<vkb::shader>> {
                log_info("Compiling shader: {} from {}", name, path);
                return {name, vkb::shader::compile(std::move(path))};
            }, std::move(name), path.string()));
        }
        bool all_successful = true;
        for (auto&& future : futures) {
            auto [name, shader] = future.get();
            if (!shader) {
                log_error("Failed to compile shader: {}", name);
                all_successful = false;
                continue;
            }
            m_shaders[name] = std::move(shader);
        }
        return all_successful;
    }

    auto shader_registry::get() noexcept -> shader_registry& {
        passert(s_instance != nullptr);
        return *s_instance;
    }
    auto shader_registry::init(std::string&& shader_dir) -> void {
        if (!s_instance) s_instance = std::make_unique<shader_registry>(std::move(shader_dir));
    }
    auto shader_registry::shutdown() noexcept -> void { s_instance.reset(); }
}
