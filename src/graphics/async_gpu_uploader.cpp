// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "async_gpu_uploader.hpp"
#include "vulkancore/context.hpp"

namespace soliton::graphics {
    static constexpr std::size_t k_buf_offset_round = 128;

    [[nodiscard]] static inline auto get_upload_size_quant(
        const upload_task& task
    ) noexcept -> std::size_t {
        const std::size_t ts = task.get_upload_size();
        return k_buf_offset_round
            + ((ts + k_buf_offset_round - 1)
            / k_buf_offset_round);
    }

    async_upload_base::async_upload_base(
        eastl::string&& name,
        async_upload_mgr& mgr
    ) : m_name{std::move(name)}, m_mgr{mgr} {
        m_future = std::async(std::launch::async, [this] () -> void {
            vk::FenceCreateInfo info {};
            vkcheck(vkb::vkdvc().createFence(&info, vkb::get_alloc(), &m_fence));
            reset_fence();
            prepare_cmd_buf();
            while (m_run.load()) {
                thread_func();
            }
            panic_assert(!is_working());
            destroy_cmd_buf();
            vkb::vkdvc().destroyFence(m_fence, vkb::get_alloc());
            log_info("ASync GPU uploader {} offline", m_name);
        });
        log_info("ASync GPU uploader {} online", m_name);
    }

    auto async_upload_base::start_record() -> void {
        panic_assert(m_cmd.has_value());
        m_cmd->reset();
        m_cmd->begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    }

    auto async_upload_base::end_record() -> void {
        panic_assert(m_cmd.has_value());
        m_cmd->end();
    }

    auto async_upload_base::prepare_cmd_buf() -> void {
        panic_assert(m_pool_async == VK_NULL_HANDLE);
        panic_assert(!m_cmd.has_value());

        const std::uint32_t queue_idx = vkb::dvc().get_transfer_queue_idx();
        const vk::Queue queue = vkb::dvc().get_transfer_queue();
        constexpr vk::QueueFlagBits queue_flags = vk::QueueFlagBits::eTransfer;

        vk::CommandPoolCreateInfo pool_info {};
        pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        pool_info.queueFamilyIndex = queue_idx;
        vkcheck(vkb::vkdvc().createCommandPool(&pool_info, vkb::get_alloc(), &m_pool_async));

        vk::CommandBufferAllocateInfo alloc_info {};
        alloc_info.level = vk::CommandBufferLevel::ePrimary;
        alloc_info.commandPool = m_pool_async;
        alloc_info.commandBufferCount = 1;

        vk::CommandBuffer cmd {};
        vkcheck(vkb::vkdvc().allocateCommandBuffers(&alloc_info, &cmd));

        m_cmd.emplace(
            m_pool_async,
            cmd,
            queue,
            queue_flags
        );
    }

    auto async_upload_base::destroy_cmd_buf() -> void {
        panic_assert(m_cmd.has_value());
        panic_assert(m_pool_async != VK_NULL_HANDLE);
        vkb::vkdvc().freeCommandBuffers(m_pool_async, 1, &**m_cmd);
        vkb::vkdvc().destroyCommandPool(m_pool_async, vkb::get_alloc());
        m_cmd.reset();
        m_pool_async = VK_NULL_HANDLE;
    }

    auto async_upload_base::on_finish() -> void {
        panic_assert(m_is_working);
        m_is_working.store(false);
    }

    auto async_upload_base::reset_fence() -> void {
        vkcheck(vkb::vkdvc().resetFences(1, &m_fence));
    }

    auto async_upload_base::await() -> void {
        m_future.wait();
    }

    auto async_upload_base::stop() -> void {
        m_run.store(false);
    }

    dynamic_async_uploader::dynamic_async_uploader(
        eastl::string&& name,
        async_upload_mgr& mgr
    ) : async_upload_base{std::move(name), mgr} {}

    auto dynamic_async_uploader::load_tick() -> void {

    }

    auto dynamic_async_uploader::try_release_stage_buf() -> void {

    }

    auto dynamic_async_uploader::thread_func() -> void {

    }

    auto dynamic_async_uploader::on_finish() -> void {
        async_upload_base::on_finish();
    }


    static_async_uploader::static_async_uploader(eastl::string&& name, async_upload_mgr& mgr) : async_upload_base {std::move(name), mgr} {

    }

    auto static_async_uploader::on_finish() -> void {
        async_upload_base::on_finish();
    }

    auto static_async_uploader::load_tick() -> void {

    }

    auto static_async_uploader::thread_func() -> void {

    }

    async_upload_mgr::async_upload_mgr(
        const std::size_t static_uploaders_count,
        const std::size_t dyamic_uploaders_count,
        const std::size_t static_uploaders_max_size,
        const std::size_t dynamic_uploaders_min_size
    ) : m_static_uploaders_max_size{static_uploaders_max_size << 20},
        m_dynamic_uploaders_min_size{dynamic_uploaders_min_size << 20} {
        log_info("Initializing ASync GPU upload manager. Static uploaders: {}, Dynamic uploaders: {}", static_uploaders_count, dyamic_uploaders_count);
        for (std::size_t i = 0; i < static_uploaders_count; ++i) {
            m_static_uploaders.emplace_back(eastl::make_unique<static_async_uploader>(
                fmt::format("Static ASync uploader {}", i).c_str(),
                *this
            ));
        }
        for (std::size_t i = 0; i < dyamic_uploaders_count; ++i) {
            m_dynamic_uploaders.emplace_back(eastl::make_unique<dynamic_async_uploader>(
                fmt::format("Dynamic ASync uploader {}", i).c_str(),
                *this
            ));
        }
    }

    auto async_upload_mgr::push_task(eastl::shared_ptr<upload_task>&& task) -> void {
        if (get_upload_size_quant(*task) >= m_dynamic_uploaders_min_size) {
            dynamic_tasks_action([&task] (auto& tasks) {
                tasks.push(std::move(task));
            });
        } else {
            static_tasks_action([&task] (auto& tasks) {
                tasks.push(std::move(task));
            });
        }
    }

    auto async_upload_mgr::tick() -> void {
        sync_pending_objects();
        submit_objects();
        if (!is_static_load_asset_task_empty())
            m_static_ctx.cv.notify_all();
        if (!is_dynamic_load_asset_task_empty())
            m_dynamic_ctx.cv.notify_all();
    }

    auto async_upload_mgr::submit_objects() -> void {
        std::unique_lock lock {m_submit_mtx};
        if (m_submit_queue.empty()) return;
        std::size_t index_queue = 0;
        std::size_t max_index = 128 - 1; // TODO
        for (auto* const obj : m_submit_queue) {
            vk::SubmitInfo submit_info {};
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &*obj->get_command_buf();
            // TODO submit
            ++index_queue;
            if (index_queue > max_index)
                index_queue = 0;
            m_pending_queue.emplace_back(obj);
        }
        m_submit_queue.clear();
    }

    auto async_upload_mgr::sync_pending_objects() -> void {
        eastl::erase_if(m_pending_queue, [] (async_upload_base* const obj) -> bool {
           bool result = false;
           if (vkb::vkdvc().getFenceStatus(obj->get_fence()) == vk::Result::eSuccess) {
               obj->on_finish();
               obj->reset_fence();
               result = true;
           }
           return result;
        });
    }

    auto async_upload_mgr::flush_task() -> void {
        while (is_busy()) {
            m_static_ctx.cv.notify_all();
            m_dynamic_ctx.cv.notify_all();
            submit_objects();
            sync_pending_objects();
        }
    }

    auto async_upload_mgr::push_submit_functions(async_upload_base* obj) -> void {
        std::unique_lock lock {m_submit_mtx};
        m_submit_queue.emplace_back(obj);
    }

    auto async_upload_mgr::is_static_load_asset_task_empty() -> bool {
        std::unique_lock lock {m_static_ctx.mtx};
        return m_static_ctx.tasks_queue.empty();
    }

    auto async_upload_mgr::is_dynamic_load_asset_task_empty() -> bool {
        std::unique_lock lock {m_dynamic_ctx.mtx};
        return m_dynamic_ctx.tasks_queue.empty();
    }

    auto async_upload_mgr::static_tasks_action(eastl::function<auto (decltype(m_static_ctx.tasks_queue)&) -> void>&& hook) -> void {
        std::unique_lock lock {m_static_ctx.mtx};
        eastl::invoke(hook, m_static_ctx.tasks_queue);
    }

    auto async_upload_mgr::dynamic_tasks_action(eastl::function<auto (decltype(m_dynamic_ctx.tasks_queue)&) -> void>&& hook) -> void {
        std::unique_lock lock {m_static_ctx.mtx};
        eastl::invoke(hook, m_static_ctx.tasks_queue);
    }

    auto async_upload_mgr::before_release_flush() -> void {
        {
            std::unique_lock lock {m_static_ctx.mtx};
            m_static_ctx.tasks_queue = {};
        }
        {
            std::unique_lock lock {m_dynamic_ctx.mtx};
            m_dynamic_ctx.tasks_queue = {};
        }
        vkcheck(vkb::vkdvc().waitIdle());
    }

    auto async_upload_mgr::is_busy() -> bool {
        bool all_free = is_static_load_asset_task_empty()
            && is_dynamic_load_asset_task_empty();
        for (auto&& uploader : m_static_uploaders)
            all_free &= !uploader->is_working();
        for (auto&& uploader : m_dynamic_uploaders)
            all_free &= !uploader->is_working();
        return !all_free;
    }

    auto async_upload_mgr::release() -> void {
        log_info("Releasing ASync GPU upload manager");
        flush_task();
        for (auto&& uploader : m_static_uploaders)
            uploader->stop();
        m_static_ctx.cv.notify_all();
        for (auto&& uploader : m_dynamic_uploaders)
            uploader->stop();
        m_dynamic_ctx.cv.notify_all();
        for (auto&& uploader : m_static_uploaders) {
            uploader->await();
            uploader.reset();
        }
        m_static_uploaders.clear();
        for (auto&& uploader : m_dynamic_uploaders) {
            uploader->await();
            uploader.reset();
        }
        m_dynamic_uploaders.clear();
        log_info("ASync GPU uploader released");
    }
}
