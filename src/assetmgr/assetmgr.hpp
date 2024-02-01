// Copyright (c) 2022 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <array>
#include <string>
#include <filesystem>
#include <vector>

#include <ankerl/unordered_dense.h>

#include "../core/core.hpp"

enum class asset_source : std::uint8_t {
    filesystem,
    memory
};

enum class asset_category : std::uint8_t {
    icon,
    mesh,
    script,
    shader,
    texture,
    material,

    $count
};

class asset : public no_copy, public no_move {
protected:
    explicit asset(
        const asset_category category,
        const asset_source source,
        std::string&& asset_path = {}
    ) noexcept :
        m_category{category}, m_source{source}, m_asset_path{std::move(asset_path)} { }

public:
    virtual ~asset() = default;

    [[nodiscard]] auto get_category() const noexcept -> asset_category { return m_category; }
    [[nodiscard]] auto get_source() const noexcept -> asset_source { return m_source; }
    [[nodiscard]] auto get_asset_path() const noexcept -> const std::string& { return m_asset_path; }
    [[nodiscard]] auto get_approx_byte_size() const noexcept -> std::size_t { return m_approx_byte_size; }

private:
    const asset_category m_category;
    const asset_source m_source;
    const std::string m_asset_path;

protected:
    std::size_t m_approx_byte_size = 0;
};

template <typename T>
concept is_asset = std::is_base_of_v<asset, T>;

template <typename  T> requires is_asset<T>
class asset_registry final {
public:
    template <typename S>
    [[nodiscard]] static auto asset_id_from_scalar(S&& s) -> std::string {
        return fmt::format("tmp_{}", s);
    }

    explicit asset_registry(std::size_t capacity = 32) {
        m_registry.reserve(capacity);
    }

    template <typename Path, typename... Args>
    [[nodiscard]] auto load(Path&& asset_path_or_id, Args&&... args) -> T* {
        if constexpr (std::is_same_v<std::decay_t<Path>, std::string>) {
            if (m_registry.contains(asset_path_or_id)) {
                ++m_cache_hits;
                return &*m_registry[asset_path_or_id];
            }
        } else {
            const std::string key = asset_id_from_scalar(asset_path_or_id);
            if (m_registry.contains(key)) {
                ++m_cache_hits;
                return &*m_registry[key];
            }
        }
        std::string key {};
        std::unique_ptr<T> ptr = nullptr;
        if constexpr (std::is_same_v<std::decay_t<Path>, std::string>) {
            key = std::string{asset_path_or_id};
            ptr = std::make_unique<T>(std::forward<Path>(asset_path_or_id), std::forward<Args>(args)...);
        } else {
            key = asset_id_from_scalar(asset_path_or_id);
            ptr = std::make_unique<T>(std::forward<Args>(args)...);
        }
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
};

namespace assetmgr {
    [[nodiscard]] extern auto get_asset_root() -> const std::string&;
    [[nodiscard]] extern auto get_asset_dir(asset_category category) -> const std::string&;
    [[nodiscard]] extern auto get_asset_path(asset_category category, const std::string& name) -> std::string;
    [[nodiscard]] extern auto load_asset_blob_raw(const std::string& path, std::vector<std::uint8_t>& out) -> bool;
    [[nodiscard]] extern auto load_asset_text_raw(const std::string& path, std::string& out) -> bool;
    [[nodiscard]] extern auto load_asset_blob(asset_category category, const std::string& name, std::vector<std::uint8_t>& out) -> bool;
    [[nodiscard]] extern auto load_asset_text(asset_category category, const std::string& name, std::string& out) -> bool;
    inline auto load_asset_blob_or_panic(asset_category category, const std::string& name, std::vector<std::uint8_t>& out) -> void {
        if (!load_asset_blob(category, name, out)) [[unlikely]] {
            panic("Failed to load asset {} from category {}", name, static_cast<std::size_t>(category));
        }
    }
    inline auto load_asset_text_or_panic(asset_category category, const std::string& name, std::string& out) -> void {
        if (!load_asset_text(category, name, out)) [[unlikely]] {
            panic("Failed to load asset {} from category {}", name, static_cast<std::size_t>(category));
        }
    }
    [[nodiscard]] extern auto get_asset_request_count() noexcept -> std::uint64_t;
    [[nodiscard]] extern auto get_total_bytes_loaded() noexcept -> std::uint64_t;
}
