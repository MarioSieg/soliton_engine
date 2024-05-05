// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <filesystem>

#include "vulkancore/shader.hpp"
#include <ankerl/unordered_dense.h>

namespace graphics {
    class shader_registry final : public no_copy, public no_move {
    public:
        explicit shader_registry(std::string&& shader_dir);

        [[nodiscard]] auto get_shader(const std::string& name) const -> std::shared_ptr<vkb::shader>;

        // search for shaders in the shader directory and compile them all
        auto compile_all() -> bool;

        [[nodiscard]] static auto get() noexcept -> shader_registry&;
        static auto init(std::string&& shader_dir) -> void;
        static auto shutdown() noexcept -> void;

    private:
        static inline std::unique_ptr<shader_registry> s_instance {};
        const std::string m_shader_dir;
        ankerl::unordered_dense::map<std::string, std::shared_ptr<vkb::shader>> m_shaders {};
    };
}
