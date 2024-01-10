// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "prelude.hpp"

#include <fstream>

using namespace std::filesystem;

auto load_shader_program(const std::string& path) -> handle<bgfx::ShaderHandle> {
    if (!exists(path)) {
        return handle<bgfx::ShaderHandle>{};
    }
    log_info("Loading shader {}", path);
    std::ifstream file {path, std::ios::binary};
    passert(file.is_open() && file.good());
    file.unsetf(std::ios::skipws);
    file.seekg(0, std::ios::end);
    const std::streamsize size = file.tellg();
    passert(size != 0 && size <= 0xffffffffull);
    file.seekg(0, std::ios::beg);
    const auto* mem = bgfx::alloc(size);
    passert(mem != nullptr);
    file.read(reinterpret_cast<char*>(mem->data), size);
    bgfx::ShaderHandle h = bgfx::createShader(mem);
    passert(bgfx::isValid(h));
    return handle {h};
}

auto load_shader_registry(
    path&& root,
    ankerl::unordered_dense::map<std::string, handle<bgfx::ProgramHandle>>& registry
) -> void {
    passert(exists(root) && "Shader root dir not found");
    const path platform = [] {
#if PLATFORM_WINDOWS // win32
        return "windows";
#elif PLATFORM_OSX // MAC
        return "osx";
#elif PLATFORM_LINUX
        return "linux";
#else
#error "Unknown OS"
#endif
    }();
    const path api = [] {
        using enum bgfx::RendererType::Enum;
        switch (const auto type = bgfx::getRendererType(); type) {
            case Direct3D11: return "dx11";
            case Direct3D12: return "dx12";
            case Vulkan: return "vulkan";
            case Metal: return "metal";
            case OpenGL: return "gl";
            default:
                panic("Unsupported renderer: {}", bgfx::getRendererName(type));
        }
    }();
    root = root / platform / api;
    log_info("Loading shaders from {}", root.string());
    for (auto&& dir : directory_iterator{root}) {
        if (dir.exists() && dir.is_directory()) {
            const auto load_file = [root = dir.path()](path&& name) -> handle<bgfx::ShaderHandle> {
                return load_shader_program((root / name).string());
            };
            handle<bgfx::ShaderHandle> vs = load_file("vs.bin");
            handle<bgfx::ShaderHandle> fs = load_file("fs.bin");
            handle<bgfx::ShaderHandle> cs = load_file("cs.bin");
            if (vs && fs) {
                registry.emplace(dir.path().filename().string(), handle {bgfx::createProgram(vs, fs, true)});
            } else if (cs) {
                registry.emplace(dir.path().filename().string(), handle {bgfx::createProgram(cs, true)});
            } else {
                panic("No shaders found in {}", dir.path().string());
            }
        }
    }
}
