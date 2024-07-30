// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>

#include "utils.hpp"

namespace lu {
    /**
     * Bounded single-producer single-consumer wait and lock-free queue.
     * Only a single writer thread can perform enqueue operations
     * and only a single reader thread can perform dequeue operations.
     * Any other usage is invalid.
     * @tparam T
     * @tparam Alloc
     */
    template <typename T, typename Alloc = std::allocator<T>>
    struct spsc_queue final {
    private:
        static constexpr std::size_t k_hardware_destructive_interference_size = 64; // TODO: check for std::k_hardware_destructive_interference_size
        static constexpr std::size_t k_padding = (k_hardware_destructive_interference_size - 1) / sizeof(T) - 1;

        template <typename Alloc2, typename = void>
        struct alloc_least : std::false_type { };
        template <typename Alloc2>
        struct alloc_least
        <
            Alloc2, std::void_t
            <
                typename Alloc2::value_type,
                decltype
                (
                    std::declval<Alloc2&>().alloc_least(std::size_t{})
                )
            >
        > : std::true_type {};

        std::size_t m_cap {};
        T* m_slots {};
        [[no_unique_address]] Alloc m_alloc {};
        alignas(k_hardware_destructive_interference_size) std::atomic<std::size_t> m_write_idx {};
        alignas(k_hardware_destructive_interference_size) std::size_t m_read_idx_cache {};
        alignas(k_hardware_destructive_interference_size) std::atomic<std::size_t> m_read_idx {};
        alignas(k_hardware_destructive_interference_size) std::size_t m_write_idx_cache {};
        [[maybe_unused]] eastl::array<std::byte, k_hardware_destructive_interference_size - sizeof(m_write_idx_cache)> m_pad {};

    public:
        /**
         * Create a SPSCqueue holding items of type T with capacity capacity. capacity needs to be at least 1.
         * @param capacity
         * @param allocator
         */
        explicit spsc_queue(std::size_t capacity, const Alloc& allocator = Alloc {}) noexcept;
        spsc_queue(const spsc_queue&) = delete;
        spsc_queue(spsc_queue&&) = delete;
        auto operator =(const spsc_queue&) -> spsc_queue& = delete;
        auto operator =(spsc_queue&&) -> spsc_queue& = delete;
        ~spsc_queue();

        /**
         * Enqueue an item using inplace construction. Blocks if queue is full.
         * @tparam Args
         * @param args
         */
        template <typename... Args> requires std::is_constructible_v<T, Args&&...>
        auto emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args &&...>) -> void;

        /**
         * Try to enqueue an item using inplace construction. Returns true on success and false if queue is full.
         * @tparam Args
         * @param args
         * @return
         */
        template <typename... Args> requires std::is_constructible_v<T, Args&&...>
        [[nodiscard]] auto try_emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>) -> bool;

        /**
         * Enqueue an item using copy construction. Blocks if queue is full.
         * @param val
         */
        auto push_back(const T& val) noexcept(std::is_nothrow_copy_constructible_v<T>) -> void;

        /**
         * Enqueue an item using move construction.
         * Participates in overload resolution only if std::is_constructible<T, P&&>::value == true. Blocks if queue is full.
         * @tparam P
         * @param val
         */
        template <typename P, typename = typename std::enable_if<std::is_constructible_v<T, P &&>>::type>
        auto push_back(P&& val) noexcept(std::is_nothrow_constructible_v<T, P&&>) -> void;

        /**
         * Try to enqueue an item using copy construction. Returns true on success and false if queue is full.
         * @param val
         * @return
         */
        [[nodiscard]] auto try_push_back(const T& val) noexcept(std::is_nothrow_copy_constructible_v<T>) -> bool;
        template <typename P, typename = typename std::enable_if<std::is_constructible_v<T, P &&>>::type>

        /**
         * Try to enqueue an item using move construction. Returns true on success and false if queue is full.
         * Participates in overload resolution only if std::is_constructible<T, P&&>::value == true.
         * @tparam P
         * @param val
         * @return
         */
        [[nodiscard]] auto try_push_back(P&& val) noexcept(std::is_nothrow_constructible_v<T, P&&>) -> bool;

        /**
         * Return pointer to front of queue. Returns nullptr if queue is empty.
         * @return
         */
        [[nodiscard]] auto front() noexcept -> T*;

        /**
         * Dequeue first item of queue. Invalid to call if queue is empty. Requires std::is_nothrow_destructible<T>::value == true.
         */
        auto pop() noexcept -> void;

        /**
         * Returns the number of items available in the queue.
         * @return
         */
        [[nodiscard]] auto size() const noexcept -> std::size_t;

        /**
         * Returns true if queue is currently empty.
         * @return
         */
        [[nodiscard]] auto empty() const noexcept -> bool;

        /**
         * Returns the current capacity.
         * @return
         */
        [[nodiscard]] auto capacity() const noexcept -> std::size_t;
    };

    template <typename T, typename Alloc>
    spsc_queue<T, Alloc>::spsc_queue(const std::size_t capacity, const Alloc& allocator) noexcept {
        static_assert(alignof(spsc_queue<T>) == k_hardware_destructive_interference_size, "Invalid alignment");
        static_assert(sizeof(spsc_queue<T>) >= k_hardware_destructive_interference_size * 3, "Invalid size");
        this->m_cap = capacity < 1 ? 0 : capacity;
        this->m_alloc = allocator;
        ++this->m_cap;
        if (this->m_cap > eastl::numeric_limits<std::size_t>::max() - (k_padding << 1))
            this->m_cap = eastl::numeric_limits<std::size_t>::max() - (k_padding << 1);
        if constexpr(alloc_least<Alloc>::value) {
            auto res = this->m_alloc.allocate_at_least(this->m_cap + (k_padding << 1));
            this->m_slots = res.ptr;
            this->m_cap = res.count - (k_padding << 1);
        }
        else this->m_slots = std::allocator_traits<Alloc>::allocate(this->m_alloc, this->m_cap + (k_padding << 1));
        const bool is_valid = reinterpret_cast<const std::byte*>(&this->m_read_idx) - reinterpret_cast<const std::byte*>(&this->m_write_idx)
            >= static_cast<std::ptrdiff_t>(k_hardware_destructive_interference_size);
        passert(is_valid);
    }

    template <typename T, typename Alloc>
    template <typename... Args> requires std::is_constructible_v<T, Args&&...>
    auto spsc_queue<T, Alloc>::emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args &&...>) -> void
    {
        const std::size_t write_idx = this->m_write_idx.load(std::memory_order_relaxed);
        std::size_t next_write = write_idx + 1;
        if (next_write == this->m_cap) next_write = 0;
        while (this->m_read_idx_cache == next_write)
            this->m_read_idx_cache = this->m_read_idx.load(std::memory_order_acquire);
        new (this->m_slots + write_idx + k_padding) T(std::forward<Args>(args)...);
        this->m_write_idx.store(next_write, std::memory_order_release);
    }

    template <typename T, typename Alloc>
    template <typename... Args> requires std::is_constructible_v<T, Args&&...>
    auto spsc_queue<T, Alloc>::try_emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args &&...>) -> bool {
        const std::size_t write_idx = this->m_write_idx.load(std::memory_order_relaxed);
        std::size_t next_write = write_idx + 1;
        if (next_write == this->m_cap) next_write = 0;
        if (next_write == this->m_read_idx_cache) {
            this->m_read_idx_cache = this->m_read_idx.load(std::memory_order_acquire);
            if (next_write == this->m_read_idx_cache) return false;
        }
        new (this->m_slots + write_idx + k_padding) T(std::forward<Args>(args)...);
        this->m_write_idx.store(next_write, std::memory_order_release);
        return true;
    }

    template <typename T, typename Alloc>
    inline auto spsc_queue<T, Alloc>::push_back(const T& val) noexcept(std::is_nothrow_copy_constructible_v<T>) -> void {
        static_assert(std::is_copy_constructible_v<T>, "T must be copy constructible");
        this->template emplace(val);
    }

    template <typename T, typename Alloc>
    template <typename P, typename>
    inline auto spsc_queue<T, Alloc>::push_back(P&& val) noexcept(std::is_nothrow_constructible_v<T, P&&>) -> void {
        this->template emplace(std::forward<P>(val));
    }

    template <typename T, typename Alloc>
    inline auto spsc_queue<T, Alloc>::try_push_back(const T& val) noexcept(std::is_nothrow_copy_constructible_v<T>) -> bool {
        static_assert(std::is_copy_constructible_v<T>, "T must be copy constructible");
        return this->template try_emplace(val);
    }

    template <typename T, typename Alloc>
    template <typename P, typename>
    inline auto spsc_queue<T, Alloc>::try_push_back(P&& val) noexcept(std::is_nothrow_constructible_v<T, P&&>) -> bool {
        return this->template try_emplace(std::forward<P>(val));
    }

    template <typename T, typename Alloc>
    auto spsc_queue<T, Alloc>::front() noexcept -> T* {
        const std::size_t read_idx {this->m_read_idx.load(std::memory_order_relaxed) };
        if (read_idx == this->m_write_idx_cache) {
            this->m_write_idx_cache = this->m_write_idx.load(std::memory_order_acquire);
            if (this->m_write_idx_cache == read_idx) return nullptr;
        }
        return this->m_slots + read_idx + k_padding;
    }

    template <typename T, typename Alloc>
    auto spsc_queue<T, Alloc>::pop() noexcept -> void {
        static_assert(std::is_nothrow_destructible_v<T>, "T must be nothrow destructible");
        const std::size_t read_idx {this->m_read_idx.load(std::memory_order_relaxed) };
        passert(this->m_write_idx.load(std::memory_order_acquire) != read_idx);
        (*(this->m_slots + k_padding)).~T();
        std::size_t next_idx = read_idx + 1;
        if (next_idx == this->m_cap)
            next_idx = 0;
        this->m_read_idx.store(next_idx, std::memory_order_release);
    }

    template <typename T, typename Alloc>
    inline auto spsc_queue<T, Alloc>::size() const noexcept -> std::size_t {
        std::ptrdiff_t diff = this->m_write_idx.load(std::memory_order_acquire) - this->m_read_idx.load(std::memory_order_acquire);
        if (diff < 0) diff += this->m_cap;
        return static_cast<std::size_t>(diff);
    }

    template <typename T, typename Alloc>
    inline auto spsc_queue<T, Alloc>::empty() const noexcept -> bool {
        return this->m_write_idx.load(std::memory_order_acquire) == this->m_read_idx.load(std::memory_order_acquire);
    }

    template <typename T, typename Alloc>
    inline auto spsc_queue<T, Alloc>::capacity() const noexcept -> std::size_t {
        return this->m_cap - 1;
    }

    template <typename T, typename Alloc>
    spsc_queue<T, Alloc>::~spsc_queue() {
        while (this->front())
            this->pop();
        std::allocator_traits<Alloc>::deallocate(this->m_alloc, this->m_slots, this->m_cap + (k_padding << 1));
    }
}