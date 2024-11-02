// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "base.hpp"
#include "components.hpp"
#include "scene_properties.hpp"

#include "../assetmgr/assetmgr.hpp"

#include "../graphics/mesh.hpp"
#include "../graphics/texture.hpp"
#include "../graphics/material.hpp"
#include "../audio/audio_clip.hpp"

namespace lu {
    class scene : public flecs::world, public no_copy, public no_move {
    public:
        explicit scene(eastl::string&& name = {});
        virtual ~scene() override;

        const uuids::uuid id;
        scene_properties properties {};

        auto spawn(const char* name) const -> flecs::entity;
        virtual auto on_tick() -> void;
        virtual auto on_start() -> void;

        template <typename T>
        [[nodiscard]] constexpr auto get_asset_registry() -> assetmgr::asset_registry<T>&;

        flecs::entity active_camera {};

    private:
        friend class kernel;

        assetmgr::asset_registry<graphics::mesh> m_meshes {};
        assetmgr::asset_registry<graphics::texture> m_textures {};
        assetmgr::asset_registry<graphics::material> m_materials {};
        assetmgr::asset_registry<audio::audio_clip> m_audio_clips {};
    };

    template <> constexpr auto scene::get_asset_registry() -> assetmgr::asset_registry<graphics::mesh>& { return m_meshes; }
    template <> constexpr auto scene::get_asset_registry() -> assetmgr::asset_registry<graphics::texture>& { return m_textures; }
    template <> constexpr auto scene::get_asset_registry() -> assetmgr::asset_registry<graphics::material>& { return m_materials; }
    template <> constexpr auto scene::get_asset_registry() -> assetmgr::asset_registry<audio::audio_clip>& { return m_audio_clips; }
}
