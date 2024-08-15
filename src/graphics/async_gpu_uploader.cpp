// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "async_gpu_uploader.hpp"
#include "vulkancore/context.hpp"

namespace lu::graphics {
    static constexpr std::size_t k_async_static_uploader_num = 4;
    static constexpr std::size_t k_async_dynamic_uploader_num = 2;
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
            reset_fance();
            prepare_cmd_buf();
            while (m_run.load()) {
                thread_func();
            }
            passert(!is_working());
            destroy_cmd_buf();
            vkb::vkdvc().destroyFence(m_fence, vkb::get_alloc());
            log_info("ASync GPU uploader {} offline", m_name);
        });
        log_info("ASync GPU uploader {} online", m_name);
    }

    auto async_upload_base::start_record() -> void {
        passert(m_cmd.has_value());
        m_cmd->reset();
        m_cmd->begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    }

    auto async_upload_base::end_record() -> void {
        passert(m_cmd.has_value());
        m_cmd->end();
    }

    auto async_upload_base::prepare_cmd_buf() -> void {
        passert(m_pool_async == VK_NULL_HANDLE);
        passert(!m_cmd.has_value());

        vk::CommandPoolCreateInfo info {};
        info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        info.queueFamilyIndex = vkb::dvc().get_transfer_queue_idx();
        vkcheck(vkb::vkdvc().createCommandPool(&info, vkb::get_alloc(), &m_pool_async));

        m_cmd.emplace(
            // TODO
        );
    }

    auto async_upload_base::destroy_cmd_buf() -> void {

    }

    auto async_upload_base::on_finish() -> void {
        passert(m_is_working);
        m_is_working.store(false);
    }

    auto async_upload_base::reset_fance() -> void {

    }

    auto async_upload_base::await() -> void {

    }

    auto async_upload_base::stop() -> void {

    }
}
