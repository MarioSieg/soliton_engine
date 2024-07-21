// Copyright (c) 2022 Mario "Neo" Sieg. All Rights Reserved.

#include "assetmgr.hpp"
#include "stream.hpp"

#include <atomic>
#include <fstream>

#include <simdutf.h>

namespace lu::assetmgr {
    using namespace std::filesystem;

    mgr_config s_cfg;
    constinit std::atomic_size_t s_asset_requests = 0;
    constinit std::atomic_size_t s_asset_requests_failed = 0;
    constinit std::atomic_size_t s_total_bytes_loaded = 0;

    static constinit std::atomic_bool s_is_initialized = false;

    auto init(mgr_config&& cfg) -> void {
        if (s_is_initialized.load(std::memory_order_relaxed)) [[unlikely]] {
            log_error("Asset manager already initialized");
            return;
        }
        log_info("Initializing asset manager");
        s_cfg = std::move(cfg);
        log_info("Asset root directory: '{}'", s_cfg.asset_root);
        if (!std::filesystem::exists(s_cfg.asset_root)) [[unlikely]] {
            panic("Asset root directory not found: '{}'", s_cfg.asset_root);
        }
        if (s_cfg.validate_fs) [[likely]] {
            // TODO
        }
        s_is_initialized.store(true, std::memory_order_relaxed);
    }

    auto shutdown() -> void {
        if (!s_is_initialized.load(std::memory_order_relaxed)) {
            return;
        }
        // Print asset manager infos
        log_info("--------- Asset Mgr Stats --------- ");
        log_info("  Total assets requests: {}", assetmgr::get_asset_request_count());
        log_info(
            "  Total data loaded: {:.03f} GiB",
             static_cast<double>(assetmgr::get_total_bytes_loaded()) / std::pow(1024.0, 3.0)
        );
        s_is_initialized.store(false, std::memory_order_relaxed);
    }

    auto cfg() noexcept -> const mgr_config& {
        return s_cfg;
    }

    auto validate_path(std::string& full_path) -> bool {
        const simdutf::encoding_type encoding = simdutf::autodetect_encoding(full_path.c_str(), full_path.size());
        if (encoding != simdutf::encoding_type::UTF8) [[unlikely]] { /* UTF8 != ASCII but UTF8 can store ASCII */
            log_warn("Asset path {} is not ASCII encoded", full_path);
            return false;
        }
        if (!simdutf::validate_ascii(full_path.c_str(), full_path.size())) [[unlikely]] {
            log_warn("Asset path {} contains non-ASCII characters", full_path);
            return false;
        }
        if (std::ranges::find(full_path, '\\') != full_path.cend()) { // Windows path
            std::ranges::replace(full_path, '\\', '/'); // Convert to Unix path
        }
        if (!exists(full_path)) [[unlikely]] {
            log_warn("Asset path not found: {}", full_path);
            return false;
        }
        if (is_directory(full_path)) [[unlikely]] {
            log_warn("Asset path is a directory: {}", full_path);
            return false;
        }
        return true;
    }

    template <typename T>
    [[nodiscard]] static auto load_asset(const std::string& path, T& out) -> bool {
        auto stream = file_stream::open(std::string{path});
        if (!stream) [[unlikely]] {
            return false;
        }
        return stream->read_all_bytes(out);
    }

    auto load_asset_blob_raw(const std::string& path, std::vector<std::uint8_t>& out) -> bool {
        return load_asset(path, out);
    }

    auto load_asset_text_raw(const std::string& path, std::string& out) -> bool {
        return load_asset(path, out);
    }

    auto load_asset_blob(const std::string& name, std::vector<std::uint8_t>& out) -> bool {
        return load_asset(name, out);
    }

    auto load_asset_text(const std::string& name, std::string& out) -> bool {
        return load_asset(name, out);
    }

    auto get_asset_request_count() noexcept -> std::size_t {
        return s_asset_requests.load(std::memory_order_relaxed);
    }

    auto get_total_bytes_loaded() noexcept -> std::size_t {
        return s_total_bytes_loaded.load(std::memory_order_relaxed);
    }
}
