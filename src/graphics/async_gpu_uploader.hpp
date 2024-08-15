// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <atomic>
#include <future>

#include "vulkancore/prelude.hpp"
#include "vulkancore/command_buffer.hpp"
#include "vulkancore/buffer.hpp"

namespace lu::graphics {
    class upload_task {
    public:
        virtual ~upload_task() = default;
        virtual auto get_upload_size() const noexcept -> std::size_t = 0;

    protected:
        upload_task() = default;

        virtual auto on_finish() -> void = 0;
        virtual auto upload_hook(
            std::size_t stage_buf_offset,
            void* buf_start,
            vkb::command_buffer& cmd,
            vkb::buffer& stage_buf
        ) -> void = 0;
    };

    class async_upload_mgr;

    class async_upload_base : public no_copy {
    protected:
        const eastl::string m_name;
        async_upload_mgr& m_mgr;
        std::future<void> m_future {};
        std::atomic_bool m_run {};
        std::atomic_bool m_is_working {};
        vk::Fence m_fence {};
        vk::CommandPool m_pool_async {};
        eastl::optional<vkb::command_buffer> m_cmd {};

        virtual auto thread_func() -> void = 0;

        auto start_record() -> void;
        auto end_record() -> void;

    private:
        auto prepare_cmd_buf() -> void;
        auto destroy_cmd_buf() -> void;

    public:
        async_upload_base(
            eastl::string&& name,
            async_upload_mgr& mgr
        );

        virtual auto on_finish() -> void;

        auto reset_fance() -> void;
        auto await() -> void;
        auto stop() -> void;
        [[nodiscard]] auto is_working() const noexcept -> bool { return m_is_working.load(); }
        [[nodiscard]] auto get_fence() const noexcept -> const vk::Fence& { return m_fence; }
        [[nodiscard]] auto get_command_buf() const noexcept -> const vk::CommandBuffer& { return m_cmd_buf; }

    };
}
