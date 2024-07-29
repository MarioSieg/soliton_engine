// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "render_thread_pool.hpp"

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <pthread.h>
#endif

namespace lu::graphics {
    render_thread::render_thread(
        std::atomic_bool& token,
        const std::int32_t num_threads,
        const std::int32_t thread_id,
        thread_shared_ctx& shared_ctx
    ) : m_token {token}, m_num_threads {num_threads}, m_thread_id {thread_id}, m_shared_ctx {shared_ctx} {
        const vk::Device device = vkb::vkdvc();
        // create command pool
        const vk::CommandPoolCreateInfo pool_info {
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = vkb::dvc().get_graphics_queue_idx()
        };
        vkcheck(device.createCommandPool(&pool_info, vkb::get_alloc(), &m_command_pool));

        const vk::CommandBufferAllocateInfo alloc_info {
            .commandPool = m_command_pool,
            .level = vk::CommandBufferLevel::eSecondary,
            .commandBufferCount = vkb::context::k_max_concurrent_frames
        };
        vkcheck(device.allocateCommandBuffers(&alloc_info, m_command_buffers.data()));

        // start thread
        m_thread = std::thread { [=, this] () -> void { thread_routine(); }};
    }

    render_thread::~render_thread() {
        std::this_thread::sleep_for(std::chrono::milliseconds{100}); // wait for thread to finish (if it's still running
        if (m_thread.joinable()) {
            m_thread.join();
        }
        const vk::Device device = vkb::vkdvc();
        device.freeCommandBuffers(m_command_pool, vkb::context::k_max_concurrent_frames, m_command_buffers.data());
        device.destroyCommandPool(m_command_pool, vkb::get_alloc());
    }

    HOTPROC auto render_thread::thread_routine() -> void {
        log_info("Render thread {} started", m_thread_id);
        passert(m_shared_ctx.render_callback != nullptr);

#if PLATFORM_WINDOWS
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#elif PLATFORM_LINUX
        pthread_t cthr_id = pthread_self();
        pthread_setname_np(cthr_id, "Lunam Engine Render Thread");
        pthread_attr_t thr_attr {};
        int policy = 0;
        int max_prio_for_policy = 0;
        pthread_attr_init(&thr_attr);
        pthread_attr_getschedpolicy(&thr_attr, &policy);
        max_prio_for_policy = sched_get_priority_max(policy);
        pthread_setschedprio(cthr_id, max_prio_for_policy);
        pthread_attr_destroy(&thr_attr);
#endif

        while (!m_token.load(std::memory_order_relaxed)) [[likely]] {
            if (!begin_thread_frame()) [[unlikely]] {
                break;
            }
            (*m_shared_ctx.render_callback)(m_active_command_buffer, m_thread_id, m_num_threads, m_shared_ctx.usr);
            end_thread_frame();
        }

        log_info("Render thread {} stopped", m_thread_id);
    }

    auto render_thread::begin_thread_frame() -> bool {
        const std::int32_t signaled = m_shared_ctx.m_sig_render_subset.wait(true, m_num_threads);
        if (signaled < 0) [[unlikely]] {
            return false;
        }

        const vk::CommandBufferBeginInfo begin_info {
            .flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue,
            .pInheritanceInfo = m_shared_ctx.inheritance_info
        };

        m_active_command_buffer = m_command_buffers[vkb::ctx().get_current_frame()];
        vkcheck(m_active_command_buffer.begin(&begin_info));

        const auto w = static_cast<float>(vkb::ctx().get_width());
        const auto h = static_cast<float>(vkb::ctx().get_height());

        // Update dynamic viewport state
        vk::Viewport viewport {};
        viewport.width = w;
        viewport.height = -h;
        viewport.x = 0.0f;
        viewport.y = h;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        m_active_command_buffer.setViewport(0, 1, &viewport);

        // Update dynamic scissor state
        vk::Rect2D scissor {};
        scissor.extent.width = w;
        scissor.extent.height = h;
        scissor.offset.x = 0.0f;
        scissor.offset.y = 0.0f;
        m_active_command_buffer.setScissor(0, 1, &scissor);

        return true;
    }

    auto render_thread::end_thread_frame() const -> void {
        vkcheck(m_active_command_buffer.end());

        // Atomically increment the number of completed threads
        const std::int32_t completed = 1 + m_shared_ctx.m_num_threads_completed.fetch_add(1, std::memory_order_seq_cst);
        if (completed == m_num_threads) {
            m_shared_ctx.m_sig_execute_command_buffers.trigger();
        }
        m_shared_ctx.m_sig_next_frame.wait(true, m_num_threads);
        // TODO: Reset command buffer
        m_shared_ctx.m_num_threads_ready.fetch_add(1, std::memory_order_seq_cst);
        // We must wait until all threads reach this point, because
        // m_NextFrameSignal must be unsignaled before we proceed to
        // RenderSubsetSignal to avoid one thread going through the loop twice in
        // a row.
        while (m_shared_ctx.m_num_threads_ready.load(std::memory_order_seq_cst) < m_num_threads) {
            std::this_thread::yield();
        }
        passert(!m_shared_ctx.m_sig_next_frame.is_triggered());
    }

    render_thread_pool::render_thread_pool(render_bucket_callback* callback, void* usr, const std::int32_t num_threads) : m_num_threads{std::max(0, num_threads)} {
        passert(callback != nullptr);
        m_shared_ctx.render_callback = callback;
        m_shared_ctx.usr = usr;
        log_info("Creating render thread pool with {} threads", m_num_threads);
        m_threads = std::make_unique<std::optional<render_thread>[]>(m_num_threads);
        for (std::int32_t i = 0; i < m_num_threads; ++i) {
            m_threads[i].emplace(m_stop_source, m_num_threads, i, m_shared_ctx);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{100}); // wait for all threads to spin some frames
    }

    render_thread_pool::~render_thread_pool() {
        m_stop_source.store(true, std::memory_order_relaxed);
        m_shared_ctx.m_sig_render_subset.trigger(true, -1);
        m_threads.reset();
    }

    auto render_thread_pool::begin_frame(const vk::CommandBufferInheritanceInfo* inheritance_info) -> void {
        if (!m_num_threads) [[unlikely]] {
            return;
        }
        m_shared_ctx.inheritance_info = inheritance_info;
        m_shared_ctx.m_num_threads_completed.store(0, std::memory_order_seq_cst);
        m_shared_ctx.m_sig_render_subset.trigger(true);
    }

    auto render_thread_pool::process_frame(const vk::CommandBuffer primary) -> void {
        if (!m_num_threads) [[unlikely]] {
            return;
        }
        m_shared_ctx.m_sig_execute_command_buffers.wait(true, 1);
        auto* secondary_command_buffers = static_cast<vk::CommandBuffer*>(alloca(m_num_threads * sizeof(vk::CommandBuffer)));
        for (std::int32_t i = 0; i < m_num_threads; ++i) {
            secondary_command_buffers[i] = m_threads[i]->get_active_command_buffer();
        }
        primary.executeCommands(m_num_threads, secondary_command_buffers);
        m_shared_ctx.m_num_threads_ready.store(0, std::memory_order_seq_cst);
        m_shared_ctx.m_sig_next_frame.trigger(true);
    }
}
