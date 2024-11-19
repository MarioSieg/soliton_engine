// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "shader.hpp"
#include "vulkancore/context.hpp"

#include "utils/file_includer.hpp"

#include "../assetmgr/assetmgr.hpp"

namespace soliton::graphics {
    auto shader_variant::get_hash() const noexcept -> std::size_t {
        std::size_t hash = eastl::hash<eastl::string>{}(m_path);
        hash = hash_merge(hash, std::hash<std::underlying_type_t<shader_stage>>{}(static_cast<std::underlying_type_t<shader_stage>>(m_stage)));
        for (auto&& macro : m_macros) {
            hash = hash_merge(hash, eastl::hash<std::decay_t<decltype(macro)>>{}(macro));
        }
        for (auto&& [k, v] : m_macro_values) {
            hash = hash_merge(hash, eastl::hash<std::decay_t<decltype(k)>>{}(k));
            hash = hash_merge(hash, eastl::hash<std::decay_t<decltype(v)>>{}(v));
        }
        return hash;
    }

    shader::~shader() noexcept {
        if (m_reflection) {
            spvReflectDestroyShaderModule(&m_reflection.value());
            m_reflection.reset();
        }
        vkb::vkdvc().destroyShaderModule(m_module, vkb::get_alloc());
    }
}
