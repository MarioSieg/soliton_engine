// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "base.hpp"
#include "components.hpp"

#include "../assetmgr/assetmgr.hpp"

#include "../graphics/mesh.hpp"
#include "../graphics/texture.hpp"
#include "../graphics/material.hpp"

class scene : public flecs::world, public no_copy, public no_move {
public:
    const std::uint32_t id;
    std::string name = {};
    virtual ~scene() override;

    static auto new_active(std::string&& name, std::string&& file, float scale = 1.0f) -> void;
    [[nodiscard]] static auto get_active() noexcept -> const std::unique_ptr<scene>& { passert(m_active != nullptr); return m_active; }

    virtual auto on_tick() -> void;
    virtual auto on_start() -> void;

    auto spawn(const char* name) const -> flecs::entity;

    template <typename A>
    [[nodiscard]] auto get_asset_registry() -> asset_registry<A>& {
        if constexpr (std::is_same_v<A, graphics::mesh>) { return m_meshes; }
        else if constexpr (std::is_same_v<A, graphics::texture>) { return m_textures; }
        else if constexpr (std::is_same_v<A, graphics::material>) { return m_materials; }
        else { panic("Unknown asset type!"); }
    }

private:
    auto import_from_file(const std::string& path, float scale) -> void;

    asset_registry<graphics::mesh> m_meshes {};
    asset_registry<graphics::texture> m_textures {};
    asset_registry<graphics::material> m_materials {};

    friend struct proxy;
    static inline constinit std::unique_ptr<scene> m_active {};
    scene();
};
