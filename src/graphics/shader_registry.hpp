// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <filesystem>

#include "vulkancore/shader.hpp"
#include <ankerl/unordered_dense.h>

namespace graphics {
    class shader_registry final : public no_copy, public no_move {
    public:
        explicit shader_registry(std::string&& shader_dir) noexcept
            : m_shader_dir{std::move(shader_dir)} {
            passert(std::filesystem::exists(m_shader_dir));
        }

        [[nodiscard]] auto get_shader(const std::string& name) const noexcept -> std::shared_ptr<vkb::shader> {
            if (!m_shaders.contains(name)) [[unlikely]] {
                panic("Shader not within registry: {}", name);
            }
            return m_shaders.at(name);
        }

        // search for shaders in the shader directory and compile them all
        auto compile_all() -> bool;

        [[nodiscard]] static auto get() noexcept -> shader_registry& {
            passert(s_instance != nullptr);
            return *s_instance;
        }
        static auto init(std::string&& shader_dir) -> void {
            if (!s_instance) s_instance = std::make_unique<shader_registry>(std::move(shader_dir));
        }
        static auto shutdown() noexcept -> void { s_instance.reset(); }

    private:
        static inline std::unique_ptr<shader_registry> s_instance {};
        const std::string m_shader_dir;
        ankerl::unordered_dense::map<std::string, std::shared_ptr<vkb::shader>> m_shaders {};
    };
}
