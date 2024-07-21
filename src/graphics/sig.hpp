// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <type_traits>

#include "../core/core.hpp"

namespace lu::graphics {
    class thread_sig final : public no_copy, public no_move {
    public:
        static_assert(std::atomic_int32_t::is_always_lock_free);

        thread_sig() = default;
        ~thread_sig() = default;

        auto trigger(bool notify_all = false, std::int32_t value = 1) -> void;
        auto wait(bool auto_reset = false, std::int32_t num_threads_waiting = 0) -> std::int32_t;
        auto reset() noexcept -> void;
        [[nodiscard]] auto is_triggered() const noexcept -> bool;

    private:
        std::mutex m_mtx {};
        std::condition_variable m_cond {};
        std::atomic_int32_t m_signaled {};
        std::atomic_int32_t m_threads_awaken {};
    };

    inline auto thread_sig::trigger(const bool notify_all, const std::int32_t value) -> void {
        //  The thread that intends to modify the variable has to
        //  -> acquire a std::mutex (typically via std::lock_guard)
        //  -> perform the modification while the lock is held
        //  -> execute notify_one or notify_all on the std::condition_variable (the lock does not need to be held for notification)

        passert(value != 0); {
            std::unique_lock lock {m_mtx};
            passert(!m_signaled.load(std::memory_order_seq_cst) && !m_threads_awaken.load(std::memory_order_seq_cst));
            m_signaled.store(value, std::memory_order_seq_cst);
        }
        // Unlocking is done before notifying, to avoid waking up the waiting
        // thread only to block again (see notify_one for details)
        if (notify_all){
            m_cond.notify_all();
        } else {
            m_cond.notify_one();
        }
    }

    // WARNING!
    // If multiple threads are waiting for a signal in an infinite loop,
    // autoresetting the signal does not guarantee that one thread cannot
    // go through the loop twice. In this case, every thread must wait for its
    // own auto-reset signal or the threads must be blocked by another signal
    inline auto thread_sig::wait(const bool auto_reset, const std::int32_t num_threads_waiting) -> std::int32_t {
        //  Any thread that intends to wait on std::condition_variable has to
        //  * acquire a std::unique_lock<std::mutex>, on the SAME MUTEX as used to protect the shared variable
        //  * execute wait, wait_for, or wait_until. The wait operations atomically release the mutex
        //    and suspend the execution of the thread.
        //  * When the condition variable is notified, a timeout expires, or a spurious wakeup occurs,
        //    the thread is awakened, and the mutex is atomically reacquired:
        //    - The thread should then check the condition and resume waiting if the wake-up was spurious.
        std::unique_lock lock {m_mtx};
        if (!m_signaled.load(std::memory_order_seq_cst)) {
            m_cond.wait(lock, [this]() noexcept -> bool { return m_signaled.load(std::memory_order_seq_cst); });
        }
        const std::int32_t signaled_value = m_signaled.load(std::memory_order_seq_cst);
        const std::int32_t num_thread_awaken = 1 + m_threads_awaken.fetch_add(1, std::memory_order_seq_cst); // fetch_add returns the original value immediately preceding the addition.
        if (auto_reset) {
            passert(num_threads_waiting != 0);
            if (num_thread_awaken == num_threads_waiting) {
                m_signaled.store(0, std::memory_order_seq_cst);
                m_threads_awaken.store(0, std::memory_order_seq_cst);
            }
        }
        return signaled_value;
    }

    inline auto thread_sig::reset() noexcept -> void {
        m_signaled.store(0, std::memory_order_seq_cst);
        m_threads_awaken.store(0, std::memory_order_seq_cst);
    }

    inline auto thread_sig::is_triggered() const noexcept -> bool {
        return m_signaled.load(std::memory_order_seq_cst);
    }
}
