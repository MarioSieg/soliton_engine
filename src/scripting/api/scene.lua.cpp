// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../api_prelude.hpp"
#include "../../scene/scene.hpp"

// We need to store the entity id which is 64-bits wide inside a double which has 52-bits mantissa.
// This means we can't store the full entity id inside a double.
// We can however store the lower 52-bits of the entity id inside a double.
// This is what we do here.
// LuaJIT can only handle 64-bit integers using the FFI which will allocate each integer on the heap, which sucks.

// std::isfinite() doesn't work with -ffast-math, so we need to do this ourselves:
__attribute__((always_inline)) static constexpr auto is_double_valid_as_id(const double x) noexcept -> bool {
    return (std::bit_cast<std::uint64_t>(x) & 0x7ff0000000000000) >> 52 != 0x7ff; // Check if exponent is not 0x7ff (infinity or NaN)
}

// Static assert self test:
static_assert(is_double_valid_as_id(0.0) == true);
static_assert(is_double_valid_as_id(1.0) == true);
static_assert(is_double_valid_as_id(2.0) == true);
static_assert(is_double_valid_as_id(-3.0) == true);
static_assert(is_double_valid_as_id(std::numeric_limits<double>::quiet_NaN()) == false);
static_assert(is_double_valid_as_id(std::numeric_limits<double>::infinity()) == false);

__attribute__((always_inline)) static auto entity_id_to_double(const id_t id) noexcept -> double {
    const auto d = std::bit_cast<double>(id);
    passert(is_double_valid_as_id(d) && "Malformed entity id");
    return d;
}

__attribute__((always_inline)) static auto double_to_entity_id(const double d) noexcept -> id_t {
    passert(is_double_valid_as_id(d) && "Malformed entity id");
    return std::bit_cast<id_t>(d);
}

LUA_INTEROP_API auto __lu_scene_new() -> std::uint32_t {
    scene::new_active();
    return scene::get_active()->id;
}

LUA_INTEROP_API auto __lu_scene_tick() -> void {
    if (scene::get_active() != nullptr) [[likely]] {
        scene::get_active()->on_tick();
    }
}

LUA_INTEROP_API auto __lu_scene_start() -> void {
    if (scene::get_active() != nullptr) [[likely]] {
        scene::get_active()->on_start();
    }
}

LUA_INTEROP_API auto __lu_scene_spawn_entity(const char* name) -> double {
    if (scene::get_active() != nullptr) [[likely]] {
        const entity entity = scene::get_active()->entity(name);
        const double d = entity_id_to_double(entity.id());
        return d;
    }
    return 0.0;
}

