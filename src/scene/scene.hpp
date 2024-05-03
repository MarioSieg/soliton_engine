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
    const int id;
    std::string name = {};
    virtual ~scene() override;

    static auto new_active(std::string&& name, std::string&& file, float scale, std::uint32_t load_flags) -> void;
    [[nodiscard]] static auto get_active() noexcept -> scene& {
        assert(s_active != nullptr);
        return *s_active;
    }

    virtual auto on_tick() -> void;
    virtual auto on_start() -> void;

    auto spawn(const char* name) const -> flecs::entity;

    template <typename A>
    [[nodiscard]] auto get_asset_registry() -> assetmgr::asset_registry<std::decay_t<A>>& {
        if constexpr (std::is_same_v<std::decay_t<A>, graphics::mesh>) { return m_meshes; }
        else if constexpr (std::is_same_v<std::decay_t<A>, graphics::texture>) { return m_textures; }
        else if constexpr (std::is_same_v<std::decay_t<A>, graphics::material>) { return m_materials; }
        else { panic("Unknown asset type!"); }
    }

    flecs::entity active_camera {};

private:
    friend class kernel;
    auto import_from_file(const std::string& path, float scale, std::uint32_t load_flags) -> void;

    assetmgr::asset_registry<graphics::mesh> m_meshes {};
    assetmgr::asset_registry<graphics::texture> m_textures {};
    assetmgr::asset_registry<graphics::material> m_materials {};

    friend struct proxy;
    static inline constinit std::unique_ptr<scene> s_active {};
    scene();
};
