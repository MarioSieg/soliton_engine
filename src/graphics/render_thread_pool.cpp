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
        const vk::Queue queue = vkb::dvc().get_graphics_queue();

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

        eastl::vector<vk::CommandBuffer> command_buffers {};
        command_buffers.resize(vkb::context::k_max_concurrent_frames);
        vkcheck(device.allocateCommandBuffers(&alloc_info, command_buffers.data()));

        m_command_buffers.reserve(command_buffers.size());
        for (const vk::CommandBuffer cmd : command_buffers) {
            m_command_buffers.emplace_back(m_command_pool, cmd, queue, vk::QueueFlagBits::eGraphics);
        }

        // start thread
        m_thread = std::thread { [=, this] () -> void { thread_routine(); }};
    }

    render_thread::~render_thread() {
        std::this_thread::sleep_for(std::chrono::milliseconds{100}); // wait for thread to finish (if it's still running)
        if (m_thread.joinable()) {
            m_thread.join();
        }
        const vk::Device device = vkb::vkdvc();
        m_command_buffers.clear();
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

        for (;;) {
            if (m_token.load(std::memory_order_relaxed)) [[unlikely]] break;
            if (!begin_thread_frame()) [[unlikely]] break;
            eastl::invoke(m_shared_ctx.render_callback, *m_active_command_buffer, m_thread_id, m_num_threads);
            end_thread_frame();
        }

        log_info("Render thread {} stopped", m_thread_id);
    }

    auto render_thread::begin_thread_frame() -> bool {
        const std::int32_t signaled = m_shared_ctx.m_sig_render_subset.wait(true, m_num_threads);
        if (signaled < 0) [[unlikely]] return false;

        m_active_command_buffer = &m_command_buffers[vkb::ctx().get_current_frame()];
        m_active_command_buffer->begin(vk::CommandBufferUsageFlagBits::eRenderPassContinue, m_shared_ctx.inheritance_info);

        const auto w = static_cast<float>(vkb::ctx().get_width());
        const auto h = static_cast<float>(vkb::ctx().get_height());

        m_active_command_buffer->set_viewport(0.0f, h, w, -h);
        m_active_command_buffer->set_scissor(w, h);

        return true;
    }

    auto render_thread::end_thread_frame() const -> void {
        m_active_command_buffer->end();

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

    render_thread_pool::render_thread_pool(eastl::function<render_bucket_callback>&& callback,const std::int32_t num_threads) : m_num_threads{std::max(0, num_threads)} {
        passert(callback != nullptr);
        m_shared_ctx.render_callback = std::move(callback);
        log_info("Creating render thread pool with {} threads", m_num_threads);
        m_threads = eastl::make_unique<eastl::optional<render_thread>[]>(m_num_threads);
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

    auto render_thread_pool::process_frame(vkb::command_buffer& primary_cmd) -> void {
        if (!m_num_threads) [[unlikely]] return;
        m_shared_ctx.m_sig_execute_command_buffers.wait(true, 1);
        auto* secondary_command_buffers = static_cast<vk::CommandBuffer*>(alloca(m_num_threads * sizeof(vk::CommandBuffer)));
        for (std::int32_t i = 0; i < m_num_threads; ++i) {
            secondary_command_buffers[i] = **m_threads[i]->get_active_command_buffer();
        }
        primary_cmd.execute_commands(eastl::span{secondary_command_buffers, static_cast<std::size_t>(m_num_threads)});
        m_shared_ctx.m_num_threads_ready.store(0, std::memory_order_seq_cst);
        m_shared_ctx.m_sig_next_frame.trigger(true);
    }
}
