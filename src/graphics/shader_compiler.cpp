// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "shader_compiler.hpp"
#include "vulkancore/context.hpp"
#include "../assetmgr/assetmgr.hpp"
#include "utils/file_includer.hpp"
#include <filesystem>

namespace soliton::graphics {
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
    auto shader_compiler::preprocess_shader(
        const std::string& source_name,
        const shaderc_shader_kind kind,
        const std::string& source,
        const shaderc::CompileOptions& options,
        std::string& out,
        std::string& out_error
    ) -> bool {
        const shaderc::PreprocessedSourceCompilationResult result = m_compiler.PreprocessGlsl(source, kind, source_name.c_str(), options);
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) [[unlikely]] {
            std::stringstream error {};
            error << source_name << ": " << get_shaderc_err(result.GetCompilationStatus());
            if (const auto msg = result.GetErrorMessage(); !msg.empty()) {
                error << "\n" << result.GetErrorMessage();
            }
            out_error = error.str();
            out = {};
            return false;
        }
        out = {result.cbegin(), result.cend()};
        return true;
    }

    // Compiles a shader to SPIR-V assembly. Returns the assembly text as a string.
    auto shader_compiler::compile_file_to_assembly(
        const std::string& source_name,
        const shaderc_shader_kind kind,
        const std::string& source,
        const shaderc::CompileOptions& options,
        eastl::string& out,
        std::string& out_error
    ) -> bool {
        const shaderc::AssemblyCompilationResult result = m_compiler.CompileGlslToSpvAssembly( source, kind, source_name.c_str(), options);
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) [[unlikely]] {
            std::stringstream error {};
            error << source_name << ": " << get_shaderc_err(result.GetCompilationStatus());
            if (const auto msg = result.GetErrorMessage(); !msg.empty()) {
                error << "\n" << result.GetErrorMessage();
            }
            out_error = error.str();
            out = {};
            return false;
        }
        out = {result.cbegin(), result.cend()};
        return true;
    }

    // Compiles a shader to a SPIR-V binary. Returns the binary as a vector of 32-bit words.
    auto shader_compiler::compile_file_to_bin(
        const std::string& source_name,
        shaderc_shader_kind kind,
        const std::string& source,
        const shaderc::CompileOptions& options,
        eastl::vector<uint32_t>& out,
        std::string& out_error
    ) -> bool {
        const shaderc::SpvCompilationResult result = m_compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) [[unlikely]] {
            std::stringstream error {};
            error << source_name << ": " << get_shaderc_err(result.GetCompilationStatus());
            if (const auto msg = result.GetErrorMessage(); !msg.empty()) {
                error << "\n" << result.GetErrorMessage();
            }
            out_error = error.str();
            out = {};
            return false;
        }
        out = {result.cbegin(), result.cend()};
        return true;
    }

    auto shader_compiler::configure_compile_options(shaderc::CompileOptions& opt, const shader_variant& variant) -> void {
        opt.SetOptimizationLevel(shaderc_optimization_level_performance);
        opt.SetSourceLanguage(shaderc_source_language_glsl);
        opt.SetIncluder(std::make_unique<graphics::FileIncluder>(&m_file_finder));
        std::uint32_t vk_version = 0;
        switch (vkb::device::k_vulkan_api_version) {
            case VK_API_VERSION_1_0: vk_version = shaderc_env_version_vulkan_1_0; break;
            case VK_API_VERSION_1_1: vk_version = shaderc_env_version_vulkan_1_1; break;
            case VK_API_VERSION_1_2: vk_version = shaderc_env_version_vulkan_1_2; break;
            case VK_API_VERSION_1_3: vk_version = shaderc_env_version_vulkan_1_3; break;
            default: panic("Unsupported Vulkan API version: {}", vkb::device::k_vulkan_api_version);
        }
        opt.SetTargetEnvironment(shaderc_target_env_vulkan, vk_version);
        opt.SetWarningsAsErrors();

        for (auto&& macro : variant.get_macros()) {
            opt.AddMacroDefinition(macro.c_str());
        }
        for (auto&& [k, v] : variant.get_keys()) {
            opt.AddMacroDefinition(k.c_str(), std::to_string(v));
        }
    }

    auto shader_compiler::compile(shader_variant&& variant) -> eastl::pair<eastl::shared_ptr<shader>, eastl::string> {
        const auto start = eastl::chrono::high_resolution_clock::now();

        // Load string BLOB from file
        eastl::string source_code_glsl {};
        bool success {};

        assetmgr::with_primary_accessor_lock([&](assetmgr::asset_accessor &acc) {
            success = acc.load_txt_file(variant.get_path().c_str(), source_code_glsl);
        });
        if (!success) [[unlikely]] {
            return {nullptr, {fmt::format("Failed to load shader file: {}", variant.get_path()).c_str()}};
        }

        shaderc::CompileOptions options {};
        configure_compile_options(options, variant);

        shaderc_shader_kind kind = shaderc_glsl_infer_from_source;
        vk::ShaderStageFlagBits vk_stage {};
        switch (variant.get_stage()) {
            case shader_stage::vertex:
                kind = shaderc_vertex_shader;
                vk_stage = vk::ShaderStageFlagBits::eVertex;
                break;
            case shader_stage::fragment:
                kind = shaderc_fragment_shader;
                vk_stage = vk::ShaderStageFlagBits::eFragment;
                break;
            case shader_stage::compute:
                kind = shaderc_compute_shader;
                vk_stage = vk::ShaderStageFlagBits::eCompute;
                break;
            default:
                panic("Unsupported shader stage: {}", static_cast<std::underlying_type_t<shader_stage>>(variant.get_stage()));
        }

        const std::string file_name = std::filesystem::path {variant.get_path().c_str()}.filename().string();
        std::string src = source_code_glsl.c_str();
        std::string error {};
        if (!preprocess_shader(file_name, kind, src, options, src, error)) [[unlikely]] {
            return {nullptr, error.c_str()};
        }

        struct proxy : shader {};
        auto shader = eastl::make_shared<proxy>();

        eastl::vector<std::uint32_t> bytecode {};
        if (!compile_file_to_bin(file_name, kind, src, options, bytecode, error) || bytecode.empty()) [[unlikely]] {
            return {nullptr, error.c_str()};
        }

        vk::ShaderModuleCreateInfo create_info {};
        create_info.codeSize = bytecode.size() * sizeof(std::uint32_t);
        create_info.pCode = bytecode.data();
        if (vk::Result::eSuccess != vkb::ctx().get_device().get_logical_device().createShaderModule(&create_info, vkb::get_alloc(), &shader->m_module)) [[unlikely]] {
            return {nullptr, error.c_str()};
        }

        if (variant.get_reflect()) {
            SpvReflectShaderModule module;
            SpvReflectResult result = spvReflectCreateShaderModule(bytecode.size() * sizeof(std::uint32_t), bytecode.data(), &module);
            if (result == SPV_REFLECT_RESULT_SUCCESS) [[likely]] {
                shader->m_reflection = module;
            } else {
                log_error("Failed to reflect shader: {}", file_name);
            }
        }
        if (variant.get_keep_source()) {
            shader->m_source = std::move(source_code_glsl);
        }
        if (variant.get_keep_bytecode()) {
            shader->m_bytecode = std::move(bytecode);
        }
        if (variant.get_keep_assembly()) {
            eastl::string assembly {};
            if (!compile_file_to_assembly(file_name, kind, src, options, assembly, error)) [[unlikely]] {
                return {nullptr, error.c_str()};
            }
            shader->m_assembly = std::move(assembly);
        }
        shader->m_variant = std::move(variant);

        vk::PipelineShaderStageCreateInfo& stage_info = shader->m_stage_info;
        stage_info.stage = vk_stage;
        stage_info.module = shader->m_module;
        stage_info.pName = shader->m_variant.get_entry_point().c_str();

        log_info("Compiled shader: {} in {:.03f}s", file_name, eastl::chrono::duration_cast<eastl::chrono::duration<double>>(eastl::chrono::high_resolution_clock::now() - start).count());

        return {shader, {}};
    }

    shader_compiler::shader_compiler() {
        for (auto&& dir : include_dirs) {
            m_file_finder.search_path().emplace_back(dir.data());
        }
        log_info("Shader compiler initialized");
    }

    shader_compiler::~shader_compiler() {
        log_info("Shader compiler shutdown");
    }

    auto shader_cache::get_shader(shader_variant&& variant) -> eastl::shared_ptr<shader> {
        const std::size_t hash = variant.get_hash();
        const bool exists = m_shaders.find(hash) != m_shaders.end();
        if (exists) return m_shaders[hash];
        if (!m_compiler) {
            m_compiler.emplace();
        }
        auto [shader, error] = m_compiler->compile(eastl::move(variant));
        if (!shader) [[unlikely]] {
            log_error("{}", error);
            return nullptr;
        }
        m_shaders[hash] = shader;
        return shader;
    }

    auto shader_cache::shutdown_compiler() -> void {
        m_compiler.reset();
    }

    auto shader_cache::invalidate_all() -> void {
        m_shaders.clear();
    }
}
