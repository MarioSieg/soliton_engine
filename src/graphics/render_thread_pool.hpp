// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <optional>

#include "vulkancore/prelude.hpp"
#include "vulkancore/context.hpp"

#include "sig.hpp"

namespace graphics {
    using render_bucket_callback = auto(const vk::CommandBuffer cmd_buf, const std::int32_t bucket_id, const std::int32_t num_threads, void* usr) -> void;

    struct thread_shared_ctx final {
        thread_sig m_sig_render_subset {};
        thread_sig m_sig_execute_command_buffers {};
        thread_sig m_sig_next_frame {};
        std::atomic_int32_t m_num_threads_completed {};
        std::atomic_int32_t m_num_threads_ready {};
        const vk::CommandBufferInheritanceInfo* inheritance_info {};
        render_bucket_callback* render_callback {};
        void* usr {};
    };

    class render_thread final : public no_copy, public no_move {
    public:
        render_thread(
            const std::stop_token& token,
            std::int32_t num_threads,
            std::int32_t thread_id,
            thread_shared_ctx& shared_ctx
        );
        ~render_thread();

        [[nodiscard]] auto get_active_command_buffer() const noexcept -> vk::CommandBuffer {
            return m_active_command_buffer;
        }

    private:
        HOTPROC auto thread_routine() -> void;
        [[nodiscard]] auto begin_thread_frame() -> bool;
        auto end_thread_frame() const -> void;

        const std::stop_token m_token;
        const std::int32_t m_num_threads;
        const std::int32_t m_thread_id;
        thread_shared_ctx& m_shared_ctx;
        std::jthread m_thread {};
        vk::CommandPool m_command_pool {};
        std::array<vk::CommandBuffer, vkb::context::k_max_concurrent_frames> m_command_buffers {};
        vk::CommandBuffer m_active_command_buffer {};
        std::uint32_t m_active_frame {};
        std::uint32_t m_current_frame {};
    };

    class render_thread_pool final : public no_copy, public no_move {
    public:
        explicit render_thread_pool(render_bucket_callback* callback, void* usr, std::int32_t num_threads);
        ~render_thread_pool();

        auto begin_frame(const vk::CommandBufferInheritanceInfo* inheritance_info) -> void;
        auto process_frame(vk::CommandBuffer primary) -> void;

    private:
        const std::int32_t m_num_threads {};
        thread_shared_ctx m_shared_ctx {};
        std::unique_ptr<std::optional<render_thread>[]> m_threads {};
        std::stop_source m_stop_source {};
    };
}
