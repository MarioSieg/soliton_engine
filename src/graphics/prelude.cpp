// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "prelude.hpp"

#include <fstream>

using namespace std::filesystem;

namespace graphics {
    auto load_shader_program(const std::string& path) -> handle<bgfx::ShaderHandle> {
        passert(exists(path));
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
        root = current_path() / root;
        passert(exists(root));
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
        // TODO
        panic("fuck you");
    }
}
