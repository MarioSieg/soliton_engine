// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"

#include "shaderc/shaderc.hpp"

namespace vkb {
    class shader : public no_copy, public no_move {
    public:
        static constexpr std::array<const std::pair<std::string_view, std::optional<shaderc_shader_kind>>, 8> k_extensions {
            std::make_pair(".vert", std::make_optional(shaderc_shader_kind::shaderc_glsl_vertex_shader)),
            std::make_pair(".tesc", std::make_optional(shaderc_shader_kind::shaderc_glsl_tess_control_shader)),
            std::make_pair(".tese", std::make_optional(shaderc_shader_kind::shaderc_tess_evaluation_shader)),
            std::make_pair(".geom", std::make_optional(shaderc_shader_kind::shaderc_geometry_shader)),
            std::make_pair(".frag", std::make_optional(shaderc_shader_kind::shaderc_fragment_shader)),
            std::make_pair(".comp", std::make_optional(shaderc_shader_kind::shaderc_compute_shader)),
            std::make_pair(".glsl", std::make_optional(shaderc_shader_kind::shaderc_glsl_infer_from_source)),
            std::make_pair(".glsli", std::nullopt)
        };

        [[nodiscard]] static auto compile(
            std::string&& file_name,
            bool keep_assembly = false,
            bool keep_source = false,
            bool keep_bytecode = false,
            const std::unordered_map<std::string, std::string>& macros = {}
        ) -> std::shared_ptr<shader>;

        virtual ~shader() noexcept;

        operator vk::ShaderModule() const noexcept { return m_module; }

        static auto init_shader_compiler() -> void;
        static auto shutdown_shader_compiler() -> void;

        [[nodiscard]] auto get_module() const noexcept -> vk::ShaderModule { return m_module; }
        [[nodiscard]] auto get_kind() const noexcept -> shaderc_shader_kind { return m_shader_kind; }
        [[nodiscard]] auto get_assembly() const noexcept -> const std::string& { return m_assembly; }
        [[nodiscard]] auto get_bytecode() const noexcept -> std::span<const std::uint32_t> { return m_bytecode; }

    private:
        shader() = default;
        shaderc_shader_kind m_shader_kind {};
        std::string m_source {};
        std::string m_assembly {};
        std::vector<std::uint32_t> m_bytecode {};
        vk::ShaderModule m_module {};
    };
}
