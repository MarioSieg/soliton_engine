// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "prelude.hpp"

#include <shaderc/shaderc.hpp>

namespace vkb {
    class shader final : public no_copy, public no_move {
    public:
        shader(
            std::string&& file_name,
            bool keep_assembly = true,
            bool keep_source = true,
            const std::unordered_map<std::string, std::string>& macros = {}
        );
        ~shader() noexcept;

        operator vk::ShaderModule() const noexcept { return m_module; }

        static auto shutdown_online_compiler() -> void;

        [[nodiscard]] auto get_module() const noexcept -> vk::ShaderModule { return m_module; }
        [[nodiscard]] auto get_kind() const noexcept -> shaderc_shader_kind { return m_shader_kind; }
        [[nodiscard]] auto get_assembly() const noexcept -> const std::string& { return m_assembly; }

    private:
        shaderc_shader_kind m_shader_kind {};
        std::string m_source {};
        std::string m_assembly {};
        vk::ShaderModule m_module {};
    };
}