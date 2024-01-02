// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "scene.hpp"
#include "../core/kernel.hpp"

struct proxy final : scene {
    template <typename... Ts>
    explicit proxy(Ts&&... args) : scene(std::forward<Ts>(args)...) {}
};

static constinit std::atomic_uint32_t id_gen = 1;

scene::scene() : id{id_gen.fetch_add(1, std::memory_order_relaxed)} {
    static const auto main_tid = std::this_thread::get_id();
    passert(main_tid == std::this_thread::get_id());
}

auto scene::new_active(std::string&& name) -> void {
    auto scene = std::make_unique<proxy>();
    scene->name = std::move(name);
    log_info("Created scene {}", scene->name);
    m_active = std::move(scene);
}

auto scene::on_tick() -> void {
    progress(static_cast<float>(kernel::get().get_delta_time()));
}

auto scene::on_start() -> void {
    kernel::get().on_new_scene_start(*this);
}
