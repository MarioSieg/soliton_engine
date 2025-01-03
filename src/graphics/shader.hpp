// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"

namespace soliton::graphics {
    enum class shader_stage {
        vertex,
        fragment,
        compute
    };

    struct shader_variant final {
    public:
        inline static const eastl::string k_shader_dir = "/RES/shaders/src/";
        shader_variant(const eastl::string& path, const shader_stage stage) noexcept : m_path{k_shader_dir + path}, m_stage{stage} {}

        [[nodiscard]] auto get_path() const noexcept -> const eastl::string& { return m_path; }
        [[nodiscard]] auto get_stage() const noexcept -> shader_stage { return m_stage; }
        [[nodiscard]] auto get_keys() const noexcept -> const ankerl::unordered_dense::map<eastl::string, std::int32_t>& { return m_macro_values; }
        [[nodiscard]] auto get_macros() const noexcept -> const ankerl::unordered_dense::set<eastl::string>& { return m_macros; }
        auto set_keep_assembly(const bool keep_assembly) noexcept -> void { m_keep_assembly = keep_assembly; }
        auto set_keep_source(const bool keep_source) noexcept -> void { m_keep_source = keep_source; }
        auto set_keep_bytecode(const bool keep_bytecode) noexcept -> void { m_keep_bytecode = keep_bytecode; }
        auto set_reflect(const bool reflect) noexcept -> void { m_reflect = reflect; }
        [[nodiscard]] auto get_keep_assembly() const noexcept -> bool { return m_keep_assembly; }
        [[nodiscard]] auto get_keep_source() const noexcept -> bool { return m_keep_source; }
        [[nodiscard]] auto get_keep_bytecode() const noexcept -> bool { return m_keep_bytecode; }
        [[nodiscard]] auto get_reflect() const noexcept -> bool { return m_reflect; }
        [[nodiscard]] auto get_entry_point() const noexcept -> const eastl::string& { return m_entry_point; }
        auto set_entry_point(const eastl::string& entry_point) noexcept -> void { m_entry_point = entry_point; }
        auto set_int(const eastl::string& key, const std::int32_t value) noexcept -> void { m_macro_values[key] = value; }
        auto set_bool(const eastl::string& key, const bool value) noexcept -> void { m_macro_values[key] = value; }
        auto set_macro(const eastl::string& macro) noexcept -> void { m_macros.insert(macro); }
        [[nodiscard]] auto get_hash() const noexcept -> std::size_t;

    private:
        friend class shader;
        shader_variant() = default;
        eastl::string m_path {};
        shader_stage m_stage {};
        bool m_keep_assembly {};
        bool m_keep_source {};
        bool m_keep_bytecode {};
        bool m_reflect {};
        eastl::string m_entry_point {"main"};
        ankerl::unordered_dense::map<eastl::string, std::int32_t> m_macro_values {};
        ankerl::unordered_dense::set<eastl::string> m_macros {};
    };

    class shader : public no_copy, public no_move {
    public:
        virtual ~shader() noexcept;

        [[nodiscard]] auto get_variant() const noexcept -> const shader_variant& { return m_variant; }
        [[nodiscard]] auto get_source() const noexcept -> const eastl::optional<eastl::string>& { return m_source; }
        [[nodiscard]] auto get_assembly() const noexcept -> const eastl::optional<eastl::string>& { return m_assembly; }
        [[nodiscard]] auto get_bytecode() const noexcept -> const eastl::optional<eastl::vector<std::uint32_t>>& { return m_bytecode; }
        [[nodiscard]] auto get_module() const noexcept -> vk::ShaderModule { return m_module; }
        [[nodiscard]] auto get_stage_info() const noexcept -> const vk::PipelineShaderStageCreateInfo& { return m_stage_info; }
        [[nodiscard]] auto get_reflection() noexcept -> eastl::optional<SpvReflectShaderModule>& { return m_reflection; }

    private:
        friend class shader_compiler;
        shader() = default;
        eastl::optional<eastl::string> m_source {};
        eastl::optional<eastl::string> m_assembly {};
        eastl::optional<eastl::vector<std::uint32_t>> m_bytecode {};
        shader_variant m_variant {};
        vk::ShaderModule m_module {};
        vk::PipelineShaderStageCreateInfo m_stage_info {};
        eastl::optional<SpvReflectShaderModule> m_reflection {};
    };
}
