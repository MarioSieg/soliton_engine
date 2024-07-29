// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "assetmgr.hpp"

#include <atomic>
#include <filesystem>

namespace lu::assetmgr {
    static std::optional<asset_accessor> s_primary_accessor {};
    static std::mutex s_mtx {};
    constinit std::atomic_size_t s_asset_requests = 0;
    constinit std::atomic_size_t s_asset_requests_failed = 0;
    constinit std::atomic_size_t s_total_bytes_loaded = 0;
    static constinit std::atomic_bool s_is_initialized = false;

    auto init() -> void {
        if (s_is_initialized.load(std::memory_order_relaxed)) [[unlikely]] {
            log_error("Asset manager already initialized");
            return;
        }
        log_info("Initializing VFS asset manager");
        for (const auto [fs, vfs] : k_vfs_mounts) {
            log_info("VFS Mounting point '{}' -> '{}", fs, vfs);
            if (!std::filesystem::exists(fs)) {
                panic("Physical asset root VFS '{}' does not exist", fs);
            }
        }
        s_is_initialized.store(true, std::memory_order_relaxed);
    }

    auto shutdown() -> void {
        if (!s_is_initialized.load(std::memory_order_relaxed)) {
            return;
        }
        s_primary_accessor.reset();
        log_info("--------- Asset Mgr Stats --------- ");
        log_info("  Total assets requests: {}", assetmgr::get_asset_request_count());
        log_info(
            "  Total data loaded: {:.03f} GiB",
             static_cast<double>(assetmgr::get_total_bytes_loaded()) / std::pow(1024.0, 3.0)
        );
        s_is_initialized.store(false, std::memory_order_relaxed);
    }

    auto get_asset_request_count() noexcept -> std::size_t {
        return s_asset_requests.load(std::memory_order_relaxed);
    }

    auto get_total_bytes_loaded() noexcept -> std::size_t {
        return s_total_bytes_loaded.load(std::memory_order_relaxed);
    }

    auto with_primary_accessor_lock(const std::function<auto(asset_accessor& acc) -> void>& callback) -> void {
        std::unique_lock lock {s_mtx};
        if (!s_primary_accessor.has_value()) // Lazy init
            s_primary_accessor.emplace();
        std::invoke(callback, *s_primary_accessor);
    }
}
