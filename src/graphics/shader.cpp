// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "shader.hpp"
#include "vulkancore/context.hpp"

#include "file_includer.hpp"

#include "../assetmgr/assetmgr.hpp"
#include <filesystem>

namespace lu::graphics {
    [[nodiscard]] static constexpr auto get_shaderc_err(const shaderc_compilation_status status) noexcept -> const char* {
        switch (status) {
            case shaderc_compilation_status_success:
                return "Success";
            case shaderc_compilation_status_invalid_stage:
                return "Shader Compilation Error: Invalid stage deduction";
            case shaderc_compilation_status_compilation_error:
                return "Shader Compilation Error: Compilation error";
            case shaderc_compilation_status_internal_error:
                return "Shader Compilation Error: Internal error (unexpected failure)";
            case shaderc_compilation_status_null_result_object:
                return "Shader Compilation Error: Null result object";
            case shaderc_compilation_status_invalid_assembly:
                return "Shader Compilation Error: Invalid assembly";
            case shaderc_compilation_status_validation_error:
                return "Shader Compilation Error: Validation error";
            case shaderc_compilation_status_transformation_error:
                return "Shader Compilation Error: Transformation error";
            case shaderc_compilation_status_configuration_error:
                return "Shader Compilation Error: Configuration error";
            default:
                return "Shader Compilation Error: Unknown status";
        }
    }

    // Returns GLSL shader source text after preprocessing.
    [[nodiscard]] static auto preprocess_shader(
        shaderc::Compiler& com,
        const std::string& source_name,
        const shaderc_shader_kind kind,
        const std::string& source,
        const shaderc::CompileOptions& options,
        std::string& out
    ) -> bool {
        const shaderc::PreprocessedSourceCompilationResult result =
            com.PreprocessGlsl(source, kind, source_name.c_str(), options);
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) [[unlikely]] {
            if (const auto msg = result.GetErrorMessage(); !msg.empty())
                log_error(result.GetErrorMessage());
            log_error("{}: {}",  get_shaderc_err(result.GetCompilationStatus()), source_name);
            return false;
        }
        out = {result.cbegin(), result.cend()};
        return true;
    }

    // Compiles a shader to SPIR-V assembly. Returns the assembly text as a string.
    [[nodiscard]] static auto compile_file_to_assembly(
        shaderc::Compiler& com,
        const std::string& source_name,
        const shaderc_shader_kind kind,
        const std::string& source,
        const shaderc::CompileOptions& options,
        std::string& out
    ) -> bool {
        const shaderc::AssemblyCompilationResult result = com.CompileGlslToSpvAssembly(
            source, kind, source_name.c_str(), options);
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) [[unlikely]] {
            if (const auto msg = result.GetErrorMessage(); !msg.empty())
                log_error(result.GetErrorMessage());
            log_error("{}: {}",  get_shaderc_err(result.GetCompilationStatus()), source_name);
            return false;
        }
        out = {result.cbegin(), result.cend()};
        return true;
    }

    // Compiles a shader to a SPIR-V binary. Returns the binary as a vector of 32-bit words.
    [[nodiscard]] static auto compile_file_to_bin(
        shaderc::Compiler& com,
        const std::string& source_name,
        shaderc_shader_kind kind,
        const std::string& source,
        const shaderc::CompileOptions& options,
        std::vector<uint32_t>& out
    ) -> bool {
        const shaderc::SpvCompilationResult result =
            com.CompileGlslToSpv(source, kind, source_name.c_str(), options);
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) [[unlikely]] {
            if (const auto msg = result.GetErrorMessage(); !msg.empty())
                log_error(result.GetErrorMessage());
            log_error("{}: {}",  get_shaderc_err(result.GetCompilationStatus()), source_name);
            return false;
        }
        out = {result.cbegin(), result.cend()};
        return true;
    }

    static const shaderc_util::FileFinder s_file_finder {
        [] {
            shaderc_util::FileFinder f {};
            for (auto&& dir : shader::k_shader_include_dirs) {
                f.search_path().emplace_back(dir);
            }
            return f;
        }()
    };

    auto shader::compile(
        std::string&& file_name,
        const bool keep_assembly,
        const bool keep_source,
        const bool keep_bytecode,
        const std::unordered_map<std::string, std::string>& macros
    ) -> std::shared_ptr<shader> {
        const auto start = std::chrono::high_resolution_clock::now();

        shaderc::Compiler compiler {};
        passert(compiler.IsValid());

        // Load string BLOB from file
        std::string buffer {};
        bool success {};
        assetmgr::with_primary_accessor_lock([&](assetmgr::asset_accessor &acc) {
            success = acc.load_txt_file(file_name.c_str(), buffer);
        });
        if (!success) [[unlikely]] {
            log_error("Failed to load shader file: {}", file_name);
            return nullptr;
        }

        shaderc::CompileOptions options {};
        options.SetOptimizationLevel(shaderc_optimization_level_performance);
        options.SetSourceLanguage(shaderc_source_language_glsl);

        options.SetIncluder(std::make_unique<graphics::FileIncluder>(&s_file_finder)); // todo make shared
        std::uint32_t vk_version = 0;
        switch (vkb::device::k_vulkan_api_version) {
            case VK_API_VERSION_1_0: vk_version = shaderc_env_version_vulkan_1_0; break;
            case VK_API_VERSION_1_1: vk_version = shaderc_env_version_vulkan_1_1; break;
            case VK_API_VERSION_1_2: vk_version = shaderc_env_version_vulkan_1_2; break;
            case VK_API_VERSION_1_3: vk_version = shaderc_env_version_vulkan_1_3; break;
            default:
                log_info("Unsupported Vulkan API version: {}", vkb::device::k_vulkan_api_version);
                return nullptr;
        }
        options.SetTargetEnvironment(shaderc_target_env_vulkan, vk_version);
        options.SetWarningsAsErrors();

        for (auto&& [k, v] : macros) {
            options.AddMacroDefinition(k, v);
        }

        shaderc_shader_kind kind = shaderc_glsl_infer_from_source;
        if (std::filesystem::path fspath {file_name}; fspath.has_extension()) { // try to infer shader kind from file extension
            const std::string ext {fspath.extension().string()};
            bool found = false;
            for (auto&& [sex, skind] : k_extensions) {
                if (ext == sex && skind) {
                    kind = *skind;
                    found = true;
                    break;
                }
            }
            if (!found) [[unlikely]] {
                log_error("Unknown shader file extension: {}", fspath.string());
                return nullptr;
            }
        }

        if (!preprocess_shader(compiler, file_name, kind, buffer, options, buffer)) [[unlikely]] {
            return nullptr;
        }

        struct proxy : shader {};
        auto shader = std::make_shared<proxy>();

        std::vector<std::uint32_t> bytecode {};
        if (!compile_file_to_bin(compiler, file_name, kind, buffer, options, bytecode) || bytecode.empty()) [[unlikely]] {
            log_error("Failed to compile shader: {}", file_name);
            return nullptr;
        }

        vk::ShaderModuleCreateInfo create_info {};
        create_info.codeSize = bytecode.size() * sizeof(std::uint32_t);
        create_info.pCode = bytecode.data();
        if (vk::Result::eSuccess != vkb::ctx().get_device().get_logical_device().createShaderModule(&create_info, vkb::get_alloc(), &shader->m_module)) [[unlikely]] {
            log_error("Failed to create shader module: {}", file_name);
            return nullptr;
        }

        if (keep_assembly && !compile_file_to_assembly(compiler, file_name, kind, buffer, options, shader->m_assembly)) {
            return nullptr;
        }
        if (keep_source) {
            shader->m_source = std::move(buffer);
        }
        if (keep_bytecode) {
            shader->m_bytecode = std::move(bytecode);
        }

        log_info("Compiled shader: {} in {:.03f}s", file_name, std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start).count());

        return shader;
    }

    shader::~shader() noexcept {
        vkb::vkdvc().destroyShaderModule(m_module, vkb::get_alloc());
    }
}
