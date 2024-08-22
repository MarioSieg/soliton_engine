// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "base.hpp"
#include "components.hpp"

#include "../assetmgr/assetmgr.hpp"

#include "../graphics/mesh.hpp"
#include "../graphics/texture.hpp"
#include "../graphics/material.hpp"

namespace lu {
    class scene : public flecs::world, public no_copy, public no_move {
    public:
        const int id;
        eastl::string name = {};
        virtual ~scene() override;

        static auto new_active(eastl::string&& name, eastl::string&& file, float scale, std::uint32_t load_flags) -> void;
        [[nodiscard]] static auto get_active() noexcept -> scene& {
            assert(s_active != nullptr);
            return *s_active;
        }

        virtual auto on_tick() -> void;
        virtual auto on_start() -> void;

        auto spawn(const char* name) const -> flecs::entity;

        template <typename T>
        [[nodiscard]] constexpr auto get_asset_registry() -> assetmgr::asset_registry<T>&;

        flecs::entity active_camera {};

    private:
        friend class kernel;
        auto import_from_file(const eastl::string& path, float scale, std::uint32_t load_flags) -> void;

        assetmgr::asset_registry<graphics::mesh> m_meshes {};
        assetmgr::asset_registry<graphics::texture> m_textures {};
        assetmgr::asset_registry<graphics::material> m_materials {};

        friend struct proxy;
        static inline eastl::unique_ptr<scene> s_active {};
        scene();
    };

    template <>
    constexpr auto scene::get_asset_registry() -> assetmgr::asset_registry<graphics::mesh>& {
        return m_meshes;
    }

    template <>
    constexpr auto scene::get_asset_registry() -> assetmgr::asset_registry<graphics::texture>& {
        return m_textures;
    }

    template <>
    constexpr auto scene::get_asset_registry() -> assetmgr::asset_registry<graphics::material>& {
        return m_materials;
    }
}
