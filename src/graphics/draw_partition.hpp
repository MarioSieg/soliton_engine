// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/core.hpp"

namespace lu::graphics {
    [[nodiscard]] inline auto compute_render_bucket_range(
        const std::size_t id,
        const std::size_t num_entities,
        const std::size_t num_threads
    ) noexcept -> eastl::array<std::size_t, 2> {
        const std::size_t base_bucket_size = num_entities / num_threads;
        const std::size_t num_extra_entities = num_entities % num_threads;
        const std::size_t begin = base_bucket_size*id + eastl::min(id, num_extra_entities);
        const std::size_t end = begin + base_bucket_size + (id < num_extra_entities ? 1 : 0);
        passert(begin <= end && end <= num_entities);
        return {begin, end};
    }

    template <typename F> requires std::is_invocable_r_v<void, F, std::size_t, const com::transform&, const com::mesh_renderer&>
    inline auto partitioned_draw(
        const std::int32_t bucket_id,
        const std::int32_t num_threads,
        const eastl::vector<eastl::pair<eastl::span<const com::transform>, eastl::span<const com::mesh_renderer>>>& render_data,
        F&& callback
    ) -> void {
        static constexpr auto accumulator = [](const std::size_t acc, const auto& pair) noexcept {
            passert(pair.first.size() == pair.second.size());
            return acc + pair.first.size();
        };
        const std::size_t total_entities = std::accumulate(render_data.begin(), render_data.end(), 0, accumulator);
        // Compute start and end for this thread across all entities
        const auto [global_start, global_end] = compute_render_bucket_range(bucket_id, total_entities, num_threads);
        std::size_t processed_entities = 0;
        for (const auto& [transforms, renderers] : render_data) {
            if (processed_entities >= global_end) break; // Already processed all entities this thread is responsible for
            std::size_t local_start = 0;
            std::size_t local_end = transforms.size();
            if (processed_entities < global_start)
                local_start = std::min(transforms.size(), global_start - processed_entities);
            if (processed_entities + transforms.size() > global_end)
                local_end = std::min(transforms.size(), global_end - processed_entities);
            for (std::size_t i = local_start; i < local_end; ++i) {
                eastl::invoke(callback, i, transforms[i], renderers[i]);
            }
            processed_entities += transforms.size();
        }
    }

    inline auto is_last_thread(const std::int32_t thread_id, const std::int32_t num_threads) noexcept -> bool {
        return thread_id == num_threads - 1;
    }
}
