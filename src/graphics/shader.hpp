// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"

#include "shaderc/shaderc.hpp"

#include <ankerl/unordered_dense.h>

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <future>
#include <optional>

namespace lu::graphics {
    enum class shader_stage {
        vertex,
        fragment,
        compute
    };

    struct shader_variant final {
    public:
        shader_variant(std::string&& path, const shader_stage stage) noexcept : m_path{std::move(path)}, m_stage{stage} {}

        [[nodiscard]] inline auto get_path() const noexcept -> const std::string& { return m_path; }
        [[nodiscard]] inline auto get_stage() const noexcept -> shader_stage { return m_stage; }
        [[nodiscard]] inline auto get_keys() const noexcept -> const ankerl::unordered_dense::map<std::string, std::int32_t>& { return m_macro_values; }
        [[nodiscard]] inline auto get_macros() const noexcept -> const ankerl::unordered_dense::set<std::string>& { return m_macros; }
        inline auto set_keep_assembly(const bool keep_assembly) noexcept -> void { m_keep_assembly = keep_assembly; }
        inline auto set_keep_source(const bool keep_source) noexcept -> void { m_keep_source = keep_source; }
        inline auto set_keep_bytecode(const bool keep_bytecode) noexcept -> void { m_keep_bytecode = keep_bytecode; }
        [[nodiscard]] inline auto get_keep_assembly() const noexcept -> bool { return m_keep_assembly; }
        [[nodiscard]] inline auto get_keep_source() const noexcept -> bool { return m_keep_source; }
        [[nodiscard]] inline auto get_keep_bytecode() const noexcept -> bool { return m_keep_bytecode; }
        [[nodiscard]] inline auto get_entry_point() const noexcept -> const std::string& { return m_entry_point; }
        inline auto set_entry_point(const std::string& entry_point) noexcept -> void { m_entry_point = entry_point; }
        inline auto set_int(const std::string& key, const std::int32_t value) noexcept -> void { m_macro_values[key] = value; }
        inline auto set_bool(const std::string& key, const bool value) noexcept -> void { m_macro_values[key] = value; }
        inline auto set_macro(const std::string& macro) noexcept -> void { m_macros.insert(macro); }
        [[nodiscard]] auto get_hash() const noexcept -> std::size_t;

    private:
        friend class shader;
        shader_variant() = default;
        std::string m_path {};
        shader_stage m_stage {};
        bool m_keep_assembly : 1 {};
        bool m_keep_source : 1 {};
        bool m_keep_bytecode : 1 {};
        std::string m_entry_point {"main"};
        ankerl::unordered_dense::map<std::string, std::int32_t> m_macro_values {};
        ankerl::unordered_dense::set<std::string> m_macros {};
    };

    class shader : public no_copy, public no_move {
    public:
        static constexpr std::array<const std::string_view, 1> k_shader_include_dirs {
            "engine_assets/shaders/include"
        };

        [[nodiscard]] static auto compile(shader_variant&& variant) -> std::shared_ptr<shader>;

        virtual ~shader() noexcept;

        [[nodiscard]] auto get_variant() const noexcept -> const shader_variant& { return m_variant; }
        [[nodiscard]] auto get_source() const noexcept -> const std::optional<std::string>& { return m_source; }
        [[nodiscard]] auto get_assembly() const noexcept -> const std::optional<std::string>& { return m_assembly; }
        [[nodiscard]] auto get_bytecode() const noexcept -> const std::optional<std::vector<std::uint32_t>>& { return m_bytecode; }
        [[nodiscard]] auto get_module() const noexcept -> vk::ShaderModule { return m_module; }
        [[nodiscard]] auto get_stage_info() const noexcept -> const vk::PipelineShaderStageCreateInfo& { return m_stage_info; }

    private:
        shader() = default;
        std::optional<std::string> m_source {};
        std::optional<std::string> m_assembly {};
        std::optional<std::vector<std::uint32_t>> m_bytecode {};
        shader_variant m_variant {};
        vk::ShaderModule m_module {};
        vk::PipelineShaderStageCreateInfo m_stage_info {};
    };

    class shader_cache final : public no_copy, public no_move {
    public:
        explicit shader_cache(std::string&& shader_dir);

        [[nodiscard]] auto get_shader(shader_variant&& variant) -> std::shared_ptr<shader>;

        [[nodiscard]] static auto get() noexcept -> shader_cache&;
        static auto init(std::string&& shader_dir) -> void;
        static auto shutdown() noexcept -> void;

    private:
        static inline std::unique_ptr<shader_cache> s_instance {};

        const std::string m_shader_dir;
        ankerl::unordered_dense::map<std::size_t, std::shared_ptr<shader>> m_shaders {};
    };
}
