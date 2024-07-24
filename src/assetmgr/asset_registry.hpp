// Copyright (c) 2022 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <ankerl/unordered_dense.h>

#include "asset.hpp"

namespace lu::assetmgr {
    template <typename  T> requires is_asset<T>
    class asset_registry final {
    public:
        explicit asset_registry(std::size_t capacity = 32) {
            m_registry.reserve(capacity);
        }

        template <typename... Args>
        [[nodiscard]] auto load_from_memory(Args&&... args) -> T* {
            std::unique_ptr<T> ptr {std::make_unique<T>(std::forward<Args>(args)...)};
            ++m_cache_misses;
            return &*m_registry.emplace(fmt::format("mem_{:#X}", ++m_id_gen), std::move(ptr)).first->second;
        }

        template <typename... Args>
        [[nodiscard]] auto load(std::string&& path, Args&&... args) -> T* {
            if (m_registry.contains(path)) {
                ++m_cache_hits;
                return &*m_registry[path];
            }
            std::string key {path};
            std::unique_ptr<T> ptr {std::make_unique<T>(std::move(path), std::forward<Args>(args)...)};
            ++m_cache_misses;
            return &*m_registry.emplace(std::move(key), std::move(ptr)).first->second;
        }

        [[nodiscard]] auto get_map() const noexcept -> const ankerl::unordered_dense::map<std::string, std::unique_ptr<T>>& {
            return m_registry;
        }

        [[nodiscard]] auto get_cache_hits() const noexcept -> std::uint32_t { return m_cache_hits; }
        [[nodiscard]] auto get_cache_misses() const noexcept -> std::uint32_t { return m_cache_misses; }
        [[nodiscard]] auto get_load_count() const noexcept -> std::uint32_t { return m_cache_hits + m_cache_misses; }

        [[nodiscard]] auto invalidate() {
            m_registry.clear();
            m_cache_hits = 0;
            m_cache_misses = 0;
        }

    private:
        ankerl::unordered_dense::map<std::string, std::unique_ptr<T>> m_registry {};
        std::uint32_t m_cache_hits = 0;
        std::uint32_t m_cache_misses = 0;
        std::uint32_t m_id_gen = 0;
    };
}
