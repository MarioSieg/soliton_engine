// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "shader.hpp"
#include "context.hpp"

#include <fstream>
#include <filesystem>

#include "../../assetmgr/assetmgr.hpp"

namespace vkb {
    class shader_includer final : public shaderc::CompileOptions::IncluderInterface {
        static constexpr const char* const err = "Requested include file does not exist";

        virtual auto GetInclude(
            const char* requested_source_name,
            shaderc_include_type type,
            const char* requesting_source,
            std::size_t include_depth
        ) -> shaderc_include_result* override {
            using namespace std::filesystem;
            const std::string requested_source {assetmgr::cfg().asset_root + "/shaders/" + requested_source_name};
            log_info("Resolving shader include: {}", requested_source);
            if (type == shaderc_include_type_relative && (!exists(requested_source) || !is_regular_file(requested_source))) [[unlikely]] {
                log_error("Requested include file does not exist: {}", requested_source);
                return new shaderc_include_result {
                    .source_name = "",
                    .source_name_length = 0,
                    .content = err,
                    .content_length = std::strlen(err),
                };
            }

            static constexpr auto heap_clone = [](const std::string& str) -> std::pair<char*, std::size_t> {
                const std::size_t len = str.size();
                char* const data = new char[1+len];
                std::copy(str.begin(), str.end(), data);
                data[len] = '\0';
                return {data, len};
            };

            std::string buf {};
            assetmgr::load_asset_text_or_panic(requested_source, buf);
            const auto [src, src_len] = heap_clone(requested_source);
            const auto [data, data_len] = heap_clone(buf);
            return new shaderc_include_result {
                .source_name = src,
                .source_name_length = src_len,
                .content = data,
                .content_length = buf.size(),
            };
        }
        virtual auto ReleaseInclude(shaderc_include_result* data) -> void override {
            if (data->source_name_length) delete[] data->source_name;
            if (data->content_length && std::strncmp(data->content, err, data->content_length) != 0)
                delete[] data->content;
            delete data;
        }
    };

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
            log_error(result.GetErrorMessage());
            log_error("Failed to preprocess shader: {}", source_name);
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
            log_error(result.GetErrorMessage());
            log_error("Failed to preprocess shader: {}", source_name);
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
        const shaderc::SpvCompilationResult module =
            com.CompileGlslToSpv(source, kind, source_name.c_str(), options);
        if (module.GetCompilationStatus() != shaderc_compilation_status_success) [[unlikely]] {
            log_error(module.GetErrorMessage());
            log_error("Failed to compile shader: {}", source_name);
            return false;
        }
        out = {module.cbegin(), module.cend()};
        return true;
    }

    auto shader::compile(
        std::string&& file_name,
        const bool keep_assembly,
        const bool keep_source,
        const bool keep_bytecode,
        const std::unordered_map<std::string, std::string>& macros
    ) -> std::shared_ptr<shader> {
        const auto start = std::chrono::high_resolution_clock::now();

        shaderc::Compiler compiler {};

        // Load string BLOB from file
        std::string buffer {};
        assetmgr::load_asset_text_or_panic(file_name, buffer);

        shaderc::CompileOptions options {};
        options.SetOptimizationLevel(shaderc_optimization_level_performance);
        options.SetSourceLanguage(shaderc_source_language_glsl);
        options.SetIncluder(std::make_unique<shader_includer>());
        std::uint32_t vk_version = 0;
        switch (device::k_vulkan_api_version) {
            case VK_API_VERSION_1_0: vk_version = shaderc_env_version_vulkan_1_0; break;
            case VK_API_VERSION_1_1: vk_version = shaderc_env_version_vulkan_1_1; break;
            case VK_API_VERSION_1_2: vk_version = shaderc_env_version_vulkan_1_2; break;
            case VK_API_VERSION_1_3: vk_version = shaderc_env_version_vulkan_1_3; break;
            default:
                log_info("Unsupported Vulkan API version: {}", device::k_vulkan_api_version);
                return nullptr;
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
            else if (ext == ".glsl") { kind = shaderc_glsl_infer_from_source; }
            else { log_warn("Unsupported shader file extension: {}", ext); }
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
        if (vk::Result::eSuccess != vkb::ctx().get_device().get_logical_device().createShaderModule(&create_info, &s_allocator, &shader->m_module)) [[unlikely]] {
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
        vkb::vkdvc().destroyShaderModule(m_module, &s_allocator);
    }
}
