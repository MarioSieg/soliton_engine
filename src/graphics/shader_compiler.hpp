// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "shader.hpp"

#include "utils/file_includer.hpp"
#include <shaderc/shaderc.hpp>

namespace soliton::graphics {
    class shader_compiler : public no_copy, public no_move {
    public:
        shader_compiler();
        ~shader_compiler();

        eastl::vector<eastl::string> include_dirs {
            "engine_assets/shaders/include"
        };

        [[nodiscard]] auto compile(shader_variant&& variant) -> eastl::shared_ptr<shader>;

    private:
        shaderc_util::FileFinder m_file_finder {};
        shaderc::Compiler m_compiler {};

        // Returns GLSL shader source text after preprocessing.
        [[nodiscard]]
        auto preprocess_shader(
            const std::string& source_name,
            const shaderc_shader_kind kind,
            const std::string& source,
            const shaderc::CompileOptions& options,
            std::string& out
        ) -> bool;

        [[nodiscard]]
            auto compile_file_to_assembly(
            const std::string& source_name,
            const shaderc_shader_kind kind,
            const std::string& source,
            const shaderc::CompileOptions& options,
            eastl::string& out
        ) -> bool;

        [[nodiscard]]
        auto compile_file_to_bin(
            const std::string& source_name,
            shaderc_shader_kind kind,
            const std::string& source,
            const shaderc::CompileOptions& options,
            eastl::vector<uint32_t>& out
        ) -> bool;

        auto configure_compile_options(shaderc::CompileOptions& opt, const shader_variant& variant) -> void;
    };

    class shader_cache final : public no_copy, public no_move {
    public:
        [[nodiscard]] auto get_shader(shader_variant&& variant) -> eastl::shared_ptr<shader>;
        auto shutdown_compiler() -> void;
        auto invalidate_all() -> void;

    private:
        const eastl::string m_shader_dir;
        eastl::optional<shader_compiler> m_compiler {};
        ankerl::unordered_dense::map<std::size_t, eastl::shared_ptr<shader>> m_shaders {};
    };
}
