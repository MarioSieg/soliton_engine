// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "asset.hpp"
#include "asset_registry.hpp"
#include "stream.hpp"

#include <uuid.h>

#include "../core/core.hpp"

namespace lu::assetmgr {
    struct mgr_config final {
        std::string asset_root {};
        bool allow_standalone_asset_loading : 1 = true; // Allow loading assets from other directories than asset_root
        bool allow_source_asset_loading : 1 = true; // Allow loading assets from plain files (.png or .obj) instead of packages
        bool validate_paths : 1 = true; // Validate asset paths and UTF-8 encoding
        bool validate_fs : 1 = true; // Validate the filesystem on boot
    };

    extern auto init(mgr_config&& cfg) -> void;
    extern auto shutdown() -> void;
    [[nodiscard]] extern auto validate_path(std::string& full_path) -> bool; // validates paths and converts Windows to UNIX separators
    [[nodiscard]] extern auto cfg() noexcept -> const mgr_config&;
    [[nodiscard]] extern auto load_asset_blob_raw(const std::string& path, std::vector<std::uint8_t>& out) -> bool;
    [[nodiscard]] extern auto load_asset_text_raw(const std::string& path, std::string& out) -> bool;
    inline auto load_asset_blob_raw_or_panic(const std::string& name, std::vector<std::uint8_t>& out) -> void {
        if (!load_asset_blob_raw(name, out)) [[unlikely]] {
            panic("Failed to load asset '{}'", name);
        }
    }
    inline auto load_asset_text_raw_or_panic(const std::string& name, std::string& out) -> void {
        if (!load_asset_text_raw(name, out)) [[unlikely]] {
            panic("Failed to load asset '{}'", name);
        }
    }
    [[nodiscard]] extern auto load_asset_blob(const std::string& name, std::vector<std::uint8_t>& out) -> bool;
    [[nodiscard]] extern auto load_asset_text(const std::string& name, std::string& out) -> bool;
    inline auto load_asset_blob_or_panic(const std::string& name, std::vector<std::uint8_t>& out) -> void {
        if (!load_asset_blob(name, out)) [[unlikely]] {
            panic("Failed to load asset '{}'", name);
        }
    }
    inline auto load_asset_text_or_panic(const std::string& name, std::string& out) -> void {
        if (!load_asset_text(name, out)) [[unlikely]] {
            panic("Failed to load asset '{}'", name);
        }
    }
    [[nodiscard]] extern auto get_asset_request_count() noexcept -> std::size_t;
    [[nodiscard]] extern auto get_total_bytes_loaded() noexcept -> std::size_t;
}
