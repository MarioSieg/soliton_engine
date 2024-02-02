// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "base.hpp"
#include "components.hpp"

#include "../assetmgr/assetmgr.hpp"

#include "../graphics/mesh.hpp"
#include "../graphics/texture.hpp"
#include "../graphics/material.hpp"

using lua_entity = std::uint32_t;

class scene : public flecs::world, public no_copy, public no_move {
public:
    static_assert(sizeof(unsigned) == sizeof(std::uint32_t));
    static constexpr std::size_t k_invalid_entity = ~0u;
    static constexpr std::size_t k_max_entities = ~0u-1; // Lua entity id is uint32_t!
    const std::uint32_t id;
    std::string name = {};
    virtual ~scene();

    static auto new_active(std::string&& name, std::string&& gltf_file, float scale = 1.0f) -> void;
    [[nodiscard]] static auto get_active() noexcept -> const std::unique_ptr<scene>& { passert(m_active != nullptr); return m_active; }

    virtual auto on_tick() -> void;
    virtual auto on_start() -> void;

    auto spawn(const char* name, lua_entity* l_id = nullptr) -> flecs::entity;
    [[nodiscard]] auto get_eitbl() const noexcept -> std::span<const flecs::entity> {
        return m_eitbl;
    }
    [[nodiscard]] auto lookup_entity_via_lua_id(const lua_entity e) const -> flecs::entity {
        if (e == k_invalid_entity || e >= m_eitbl.size()) [[unlikely]]
            return flecs::entity::null();
        return m_eitbl[e];
    }

private:
    auto load_from_gltf(const std::string& path, float scale) -> void;

    asset_registry<graphics::mesh> m_meshes {};
    asset_registry<graphics::texture> m_textures {};
    asset_registry<graphics::material> m_materials {};

    std::vector<flecs::entity> m_eitbl {}; // entity id translation lookaside buffer lol
    friend struct proxy;
    static inline constinit std::unique_ptr<scene> m_active {};
    scene();
};
