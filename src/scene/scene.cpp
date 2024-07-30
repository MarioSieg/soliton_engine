// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "scene.hpp"
#include "../core/kernel.hpp"
#include "../graphics/vulkancore/context.hpp"

#include <filesystem>
#include <assimp/scene.h>
#include <ankerl/unordered_dense.h>

namespace lu {
    struct proxy final : scene {
        template <typename... Ts>
        explicit proxy(Ts&&... args) : scene(std::forward<Ts>(args)...) {}
    };

    static constinit std::atomic_int id_gen = 1;

    scene::scene() : id{id_gen.fetch_add(1, std::memory_order_seq_cst)} {
        static const auto main_tid = std::this_thread::get_id();
        passert(main_tid == std::this_thread::get_id());
    }

    scene::~scene() {
        vkcheck(vkb::vkdvc().waitIdle());
        m_meshes.invalidate();
        m_textures.invalidate();
        log_info("Destroyed scene '{}', id: {}", name, id);
    }

    auto scene::new_active(eastl::string&& name, eastl::string&& file, const float scale, const std::uint32_t load_flags) -> void {
        auto scene = eastl::make_unique<proxy>();
        scene->name = std::move(name);
        log_info("Created scene '{}', id: {}", scene->name, scene->id);
        if (!file.empty()) {
            scene->import_from_file(file, scale, load_flags);
        }
        s_active = std::move(scene);
    }

    auto scene::on_tick() -> void {
        progress(static_cast<float>(kernel::get().get_delta_time()));
    }

    auto scene::on_start() -> void {
        kernel::get().on_new_scene_start(*this);
    }

    auto scene::spawn(const char* name) const -> flecs::entity {
        flecs::entity ent = this->entity(name);
        ent.add<com::metadata>();
        return ent;
    }

}