// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "scene.hpp"
#include "../core/kernel.hpp"
#include "../graphics/vulkancore/context.hpp"

#include <filesystem>
#include <assimp/scene.h>
#include <ankerl/unordered_dense.h>
#include <flecs/addons/http.h>

namespace lu {
    struct proxy final : scene {
        template <typename... Ts>
        explicit proxy(Ts&&... args) : scene(std::forward<Ts>(args)...) {}
    };

    extern thread_local std::mt19937 s_rng {std::random_device{}()};
    extern thread_local uuids::uuid_random_generator s_uuid_gen {s_rng};

    scene::scene(eastl::string&& name) : id{s_uuid_gen()} {
        properties.name = std::move(name);
        log_info("Allocated scene {} {}", uuids::to_string(id), properties.name);
        set<flecs::Rest>({});
    }

    scene::~scene() {
        vkcheck(vkb::vkdvc().waitIdle());
        m_meshes.invalidate();
        m_textures.invalidate();
        m_materials.invalidate();
        m_audio_clips.invalidate();
        log_info("Deallocated scene {} {}", uuids::to_string(id), properties.name);
    }

    auto scene::on_tick() -> void {
        progress(static_cast<float>(kernel::get().get_delta_time()));
    }

    auto scene::on_start() -> void {
        kernel::get().start_scene(*this);
    }

    auto scene::spawn(const char* const name) const -> flecs::entity {
        flecs::entity ent = this->entity(name);
        ent.add<com::metadata>();
        return ent;
    }
}