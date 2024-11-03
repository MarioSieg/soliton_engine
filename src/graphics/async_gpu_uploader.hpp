// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

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
        virtual ~async_upload_base() = default;

        virtual auto on_finish() -> void;

        auto reset_fence() -> void;
        auto await() -> void;
        auto stop() -> void;
        [[nodiscard]] auto is_working() const noexcept -> bool { return m_is_working.load(); }
        [[nodiscard]] auto get_fence() const noexcept -> const vk::Fence& { return m_fence; }
        [[nodiscard]] auto get_command_buf() const noexcept -> const vkb::command_buffer& { return *m_cmd; }
    };

    class dynamic_async_uploader final : public async_upload_base {
    private:
        eastl::shared_ptr<upload_task> m_processed_task {};
        eastl::unique_ptr<vkb::buffer> m_stage_buf {};

    protected:
        auto load_tick() -> void;
        auto try_release_stage_buf() -> void;
        virtual auto thread_func() -> void override;

    public:
        dynamic_async_uploader(
            eastl::string&& name,
            async_upload_mgr& mgr
        );
        virtual auto on_finish() -> void override;
    };

    class static_async_uploader final : public async_upload_base {
    private:
        eastl::shared_ptr<upload_task> m_processed_task {};
        eastl::unique_ptr<vkb::buffer> m_stage_buf {};

    protected:
        auto load_tick() -> void;
        virtual auto thread_func() -> void override;

    public:
        static_async_uploader(
            eastl::string&& name,
            async_upload_mgr& mgr
        );
        virtual auto on_finish() -> void override;
    };

    class async_upload_mgr final : public no_copy, public no_move {
    private:
        struct upload_ctx final {
            std::condition_variable cv {};
            std::mutex mtx {};
            eastl::queue<eastl::shared_ptr<upload_task>> tasks_queue {};
        };
        upload_ctx m_static_ctx {};
        upload_ctx m_dynamic_ctx {};
        eastl::vector<eastl::unique_ptr<static_async_uploader>> m_static_uploaders {};
        eastl::vector<eastl::unique_ptr<dynamic_async_uploader>> m_dynamic_uploaders {};
        std::size_t m_static_uploaders_max_size {};
        std::size_t m_dynamic_uploaders_min_size {};
        std::mutex m_submit_mtx {};
        eastl::vector<async_upload_base*> m_submit_queue {};
        eastl::vector<async_upload_base*> m_pending_queue {};

    public:
        async_upload_mgr(
            std::size_t static_uploaders_count,
            std::size_t dyamic_uploaders_count,
            std::size_t static_uploaders_max_size,
            std::size_t dynamic_uploaders_min_size
        );

        auto push_task(eastl::shared_ptr<upload_task>&& task) -> void;
        auto tick() -> void;
        auto submit_objects() -> void;
        auto sync_pending_objects() -> void;
        auto flush_task() -> void;
        auto push_submit_functions(async_upload_base* obj) -> void;
        [[nodiscard]] auto is_static_load_asset_task_empty() -> bool;
        [[nodiscard]] auto is_dynamic_load_asset_task_empty() -> bool;
        auto static_tasks_action(
            eastl::function<auto (decltype(m_static_ctx.tasks_queue)&) -> void>&& hook
        ) -> void;
        auto dynamic_tasks_action(
            eastl::function<auto (decltype(m_dynamic_ctx.tasks_queue)&) -> void>&& hook
        ) -> void;
        auto before_release_flush() -> void;
        [[nodiscard]] auto is_busy() -> bool;
        auto release() -> void;
        [[nodiscard]] auto get_static_ctx() noexcept -> upload_ctx& { return m_static_ctx; }
        [[nodiscard]] auto get_dynamic_ctx() noexcept -> upload_ctx& { return m_dynamic_ctx; }
    };
}
