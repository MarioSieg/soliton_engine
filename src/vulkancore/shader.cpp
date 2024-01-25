// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "shader.hpp"
#include "context.hpp"

#include <fstream>
#include <filesystem>

#include "../assetmgr/assetmgr.hpp"

namespace vkb {
    static constinit std::optional<shaderc::Compiler> s_compiler;
    static constinit std::atomic_bool s_initialized;
    static std::mutex s_mutex;

    // Returns GLSL shader source text after preprocessing.
    static auto preprocess_shader(
        const std::string& source_name,
        const shaderc_shader_kind kind,
        const std::string& source,
        const shaderc::CompileOptions& options,
        std::string& out
    ) -> void{
        passert(s_compiler);
        const shaderc::PreprocessedSourceCompilationResult result =
            s_compiler->PreprocessGlsl(source, kind, source_name.c_str(), options);
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) [[unlikely]] {
            log_error(result.GetErrorMessage());
            panic("Failed to preprocess shader: {}", source_name);
        }
        out = {result.cbegin(), result.cend()};
    }

    // Compiles a shader to SPIR-V assembly. Returns the assembly text as a string.
    static auto compile_file_to_assembly(
        const std::string& source_name,
        const shaderc_shader_kind kind,
        const std::string& source,
        const shaderc::CompileOptions& options,
        std::string& out
    ) -> void {
        passert(s_compiler);
        const shaderc::AssemblyCompilationResult result = s_compiler->CompileGlslToSpvAssembly(
            source, kind, source_name.c_str(), options);
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) [[unlikely]] {
            log_error(result.GetErrorMessage());
            panic("Failed to preprocess shader: {}", source_name);
        }
        out = {result.cbegin(), result.cend()};
    }

    // Compiles a shader to a SPIR-V binary. Returns the binary as a vector of 32-bit words.
    static auto compile_file_to_bin(
        const std::string& source_name,
        shaderc_shader_kind kind,
        const std::string& source,
        const shaderc::CompileOptions& options,
        std::vector<uint32_t>& out
    ) -> void {
        passert(s_compiler);
        const shaderc::SpvCompilationResult module =
            s_compiler->CompileGlslToSpv(source, kind, source_name.c_str(), options);
        if (module.GetCompilationStatus() != shaderc_compilation_status_success) [[unlikely]] {
            log_error(module.GetErrorMessage());
            panic("Failed to preprocess shader: {}", source_name);
        }
        out = {module.cbegin(), module.cend()};
    }

    shader::shader(
        std::string&& file_name,
        const bool keep_assembly,
        const bool source,
        const std::unordered_map<std::string, std::string>& macros
    ) {
        if (!s_initialized) {
            log_info("Initializing online shader compiler...");
            std::lock_guard<std::mutex> lock{s_mutex};
            s_compiler.emplace();
            s_initialized = true;
        }

        const auto start = std::chrono::high_resolution_clock::now();

        // Load string BLOB from file
        std::string buffer {};
        assetmgr::load_asset_text_or_panic(asset_category::shader, file_name, buffer);

        shaderc::CompileOptions options {};
        options.SetOptimizationLevel(shaderc_optimization_level_performance);
        options.SetSourceLanguage(shaderc_source_language_glsl);
        std::uint32_t vk_version = 0;
        switch (device::k_vulkan_api_version) {
            case VK_API_VERSION_1_0: vk_version = shaderc_env_version_vulkan_1_0; break;
            case VK_API_VERSION_1_1: vk_version = shaderc_env_version_vulkan_1_1; break;
            case VK_API_VERSION_1_2: vk_version = shaderc_env_version_vulkan_1_2; break;
            case VK_API_VERSION_1_3: vk_version = shaderc_env_version_vulkan_1_3; break;
            default:
                panic("Unsupported Vulkan API version: {}", device::k_vulkan_api_version);
        }
        options.SetTargetEnvironment(shaderc_target_env_vulkan, vk_version);
        options.SetWarningsAsErrors();

        for (auto&& [k, v] : macros) {
            options.AddMacroDefinition(k, v);
        }

        shaderc_shader_kind kind = shaderc_glsl_infer_from_source;
        if (std::filesystem::path fspath {file_name}; fspath.has_extension()) { // try to infer shader kind from file extension
            if (std::string ext {fspath.extension().string()}; ext == ".vert") { kind = shaderc_glsl_vertex_shader; }
            else if (ext == ".tesc") { kind = shaderc_glsl_tess_control_shader; }
            else if (ext == ".tese") { kind = shaderc_glsl_tess_evaluation_shader; }
            else if (ext == ".geom") { kind = shaderc_glsl_geometry_shader; }
            else if (ext == ".frag") { kind = shaderc_glsl_fragment_shader; }
            else if (ext == ".comp") { kind = shaderc_glsl_compute_shader; }
            else { panic("Unsupported shader file extension: {}", ext); }
        }

        preprocess_shader(file_name, kind, buffer, options, buffer);

        if (keep_assembly) {
            compile_file_to_assembly(file_name, kind, buffer, options, m_assembly);
        }

        std::vector<std::uint32_t> spirv_bytecode {};
        compile_file_to_bin(file_name, kind, buffer, options, spirv_bytecode);
        passert(spirv_bytecode.size() * sizeof(std::uint32_t) % 4 == 0); // SPIR-V bytecode must be a multiple of 4 bytes

        if (source) {
            m_source = std::move(buffer);
        }

        vk::ShaderModuleCreateInfo create_info {};
        create_info.codeSize = spirv_bytecode.size() * sizeof(std::uint32_t);
        create_info.pCode = spirv_bytecode.data();
        vkcheck(context::s_instance->get_device().get_logical_device().createShaderModule(&create_info, &s_allocator, &m_module));

        log_info("Compiled shader: {} in {:.03f}s", file_name, std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start).count());
    }

    shader::~shader() noexcept {
        context::s_instance->get_device().get_logical_device().destroyShaderModule(m_module, &s_allocator);
    }

    auto shader::shutdown_online_compiler() -> void {
        std::lock_guard<std::mutex> lock{s_mutex};
        log_info("Shutting down online shader compiler...");
        s_compiler.reset();
        s_initialized = false;
    }
}
