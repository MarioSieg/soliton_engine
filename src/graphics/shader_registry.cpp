// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "shader_registry.hpp"

#include <future>
#include <filesystem>

namespace lu::graphics {

    shader_registry::shader_registry(std::string&& shader_dir) : m_shader_dir{std::move(shader_dir)} {
        if (!std::filesystem::exists(m_shader_dir)) {
            panic("Shader directory does not exist: {}", m_shader_dir);
        }
    }

    [[nodiscard]] auto shader_registry::get_shader(const std::string& name) const -> std::shared_ptr<shader> {
        if (!m_shaders.contains(name)) [[unlikely]] {
            panic("Shader not within registry: {}", name);
        }
        return m_shaders.at(name);
    }

    auto shader_registry::compile_all(const bool parallel) -> bool {
        using namespace std::filesystem;
        const auto now = std::chrono::high_resolution_clock::now();
        if (!parallel) [[unlikely]] {
            log_warn("Parallel shader compilation disabled - compiling shaders sequentially");
        }
        m_shaders.clear();
        std::vector<std::future<std::tuple<std::string, std::string, std::shared_ptr<shader>>>> futures {};
        for (auto&& entry : recursive_directory_iterator{m_shader_dir}) {
            if (entry.is_directory()) continue;
            const auto& path = entry.path();
            if (const auto ex = path.extension(); ex == ".h" || [&ex] {
                for (auto&& [sex, _] : shader::k_extensions) {
                    if (sex == ex) {
                        return false;
                    }
                }
                return true;
            }()) { // ignore shader headers or unknown files
                continue;
            }
            auto name = path.filename().string();
            futures.emplace_back(std::async(parallel ? std::launch::async : std::launch::deferred, [](std::string&& name, std::string&& path) {
                log_info("Compiling shader: {} from {}", name, path);
                return std::make_tuple(name, std::string{path}, shader::compile(std::move(path)));
            }, std::move(name), "/" + path.string()));
        }
        bool all_successful = true;
        for (auto&& future : futures) {
            auto [name, path, shader] = future.get();
            if (!shader) {
                log_error("Failed to compile shader: {}", path);
                all_successful = false;
                continue;
            }
            m_shaders[name] = std::move(shader);
        }
        log_info("Compiled {} shaders in {:.03f}ms", m_shaders.size(), std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - now).count());
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
