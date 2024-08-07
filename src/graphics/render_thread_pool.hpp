// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"
#include "vulkancore/context.hpp"
#include "vulkancore/command_buffer.hpp"

namespace lu::graphics {
    using render_bucket_callback = auto (
        vkb::command_buffer& cmd,
        const std::int32_t bucket_id,
        const std::int32_t num_threads
    ) -> void;

    struct thread_shared_ctx final {
        thread_sig m_sig_render_subset {};
        thread_sig m_sig_execute_command_buffers {};
        thread_sig m_sig_next_frame {};
        std::atomic_int32_t m_num_threads_completed {};
        std::atomic_int32_t m_num_threads_ready {};
        const vk::CommandBufferInheritanceInfo* inheritance_info {};
        eastl::function<render_bucket_callback> render_callback {};
    };

    class render_thread final : public no_copy, public no_move {
    public:
        render_thread(
            std::atomic_bool& token,
            std::int32_t num_threads,
            std::int32_t thread_id,
            thread_shared_ctx& shared_ctx
        );
        ~render_thread();

        [[nodiscard]] auto get_active_command_buffer() const noexcept -> vkb::command_buffer* { return m_active_command_buffer; }

    private:
        HOTPROC auto thread_routine() -> void;
        [[nodiscard]] auto begin_thread_frame() -> bool;
        auto end_thread_frame() const -> void;

        std::atomic_bool& m_token;
        const std::int32_t m_num_threads;
        const std::int32_t m_thread_id;
        thread_shared_ctx& m_shared_ctx;
        std::thread m_thread {};
        vk::CommandPool m_command_pool {};
        eastl::vector<vkb::command_buffer> m_command_buffers {};
        vkb::command_buffer* m_active_command_buffer;
    };

    class render_thread_pool final : public no_copy, public no_move {
    public:
        explicit render_thread_pool(eastl::function<render_bucket_callback>&& callback, std::int32_t num_threads);
        ~render_thread_pool();

        auto begin_frame(const vk::CommandBufferInheritanceInfo* inheritance_info) -> void;
        auto process_frame(vkb::command_buffer& primary_cmd) -> void;

    private:
        const std::int32_t m_num_threads {};
        thread_shared_ctx m_shared_ctx {};
        eastl::unique_ptr<eastl::optional<render_thread>[]> m_threads {};
        std::atomic_bool m_stop_source {};
    };
}
