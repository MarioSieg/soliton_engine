// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "shader.hpp"
#include "vulkancore/context.hpp"

#include "utils/file_includer.hpp"

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

    static const shaderc_util::FileFinder s_file_finder {
        [] {
            shaderc_util::FileFinder f {};
            for (auto&& dir : shader::k_shader_include_dirs) {
                f.search_path().emplace_back(dir);
            }
            return f;
        }()
    };

    auto shader_variant::get_hash() const noexcept -> std::size_t {
        std::size_t hash = std::hash<std::string>{}(m_path);
        hash = hash_merge(hash, std::hash<std::underlying_type_t<shader_stage>>{}(static_cast<std::underlying_type_t<shader_stage>>(m_stage)));
        for (auto&& macro : m_macros) {
            hash = hash_merge(hash, std::hash<std::decay_t<decltype(macro)>>{}(macro));
        }
        for (auto&& [k, v] : m_macro_values) {
            hash = hash_merge(hash, std::hash<std::decay_t<decltype(k)>>{}(k));
            hash = hash_merge(hash, std::hash<std::decay_t<decltype(v)>>{}(v));
        }
        return hash;
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

    auto shader::compile(shader_variant&& variant) -> std::shared_ptr<shader> {
        const auto start = std::chrono::high_resolution_clock::now();

        shaderc::Compiler compiler {};
        passert(compiler.IsValid());

        // Load string BLOB from file
        std::string source_code_glsl {};
        bool success {};
        assetmgr::with_primary_accessor_lock([&](assetmgr::asset_accessor &acc) {
            success = acc.load_txt_file(variant.get_path().c_str(), source_code_glsl);
        });
        if (!success) [[unlikely]] {
            log_error("Failed to load shader file: {}", variant.get_path());
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

        for (auto&& macro : variant.m_macros) {
            options.AddMacroDefinition(macro);
        }
        for (auto&& [k, v] : variant.m_macro_values) {
            options.AddMacroDefinition(k, std::to_string(v));
        }

        shaderc_shader_kind kind = shaderc_glsl_infer_from_source;
        vk::ShaderStageFlagBits vk_stage {};
        switch (variant.m_stage) {
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
                log_error("Unsupported shader stage: {}", static_cast<std::underlying_type_t<shader_stage>>(variant.m_stage));
                return nullptr;
        }

        const std::string file_name = std::filesystem::path{variant.get_path()}.filename().string();

        if (!preprocess_shader(compiler, file_name, kind, source_code_glsl, options, source_code_glsl)) [[unlikely]] {
            return nullptr;
        }

        struct proxy : shader {};
        auto shader = std::make_shared<proxy>();

        std::vector<std::uint32_t> bytecode {};
        if (!compile_file_to_bin(compiler, file_name, kind, source_code_glsl, options, bytecode) || bytecode.empty()) [[unlikely]] {
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

        if (variant.m_keep_source) {
            shader->m_source = std::move(source_code_glsl);
        }
        if (variant.m_keep_bytecode) {
            shader->m_bytecode = std::move(bytecode);
        }
        if (variant.m_keep_assembly) {
            std::string assembly {};
            if (!compile_file_to_assembly(compiler, file_name, kind, source_code_glsl, options, assembly)) [[unlikely]] {
                return nullptr;
            }
            shader->m_assembly = std::move(assembly);
        }
        shader->m_variant = std::move(variant);

        vk::PipelineShaderStageCreateInfo& stage_info = shader->m_stage_info;
        stage_info.stage = vk_stage;
        stage_info.module = shader->m_module;
        stage_info.pName = shader->m_variant.get_entry_point().c_str();

        log_info("Compiled shader: {} in {:.03f}s", file_name, std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start).count());

        return shader;
    }

    shader::~shader() noexcept {
        vkb::vkdvc().destroyShaderModule(m_module, vkb::get_alloc());
    }

    shader_cache::shader_cache(std::string&& shader_dir) : m_shader_dir{std::move(shader_dir)} {
        if (!std::filesystem::exists(m_shader_dir)) {
            panic("Shader directory does not exist: {}", m_shader_dir);
        }
    }

    [[nodiscard]] auto shader_cache::get_shader(shader_variant&& variant) -> std::shared_ptr<shader> {
        const std::size_t hash = variant.get_hash();
        const bool exists = m_shaders.find(hash) != m_shaders.end();
        if (exists) {
            return m_shaders[hash];
        }
        auto shader = shader::compile(std::move(variant));
        passert(shader != nullptr);
        m_shaders[hash] = shader;
        return shader;
    }

    auto shader_cache::get() noexcept -> shader_cache& {
        passert(s_instance != nullptr);
        return *s_instance;
    }
    auto shader_cache::init(std::string&& shader_dir) -> void {
        if (!s_instance) s_instance = std::make_unique<shader_cache>(std::move(shader_dir));
    }
    auto shader_cache::shutdown() noexcept -> void { s_instance.reset(); }
}
