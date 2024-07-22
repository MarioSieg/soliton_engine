// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "asset.hpp"
#include "asset_registry.hpp"
#include "asset_accessor.hpp"

#include <uuid.h>

#include "../core/core.hpp"

namespace lu::assetmgr {
    constexpr char res_path_prefix = '@';

    struct assetmgr_config final {
        std::string asset_root {};
        bool allow_standalone_asset_loading : 1 = true; // Allow loading assets from other directories than asset_root
        bool allow_source_asset_loading : 1 = true; // Allow loading assets from plain files (.png or .obj) instead of packages
        bool validate_paths : 1 = true; // Validate asset paths and UTF-8 encoding
        bool validate_fs : 1 = true; // Validate the filesystem on boot
    };

    extern auto init(assetmgr_config&& cfg) -> void;
    extern auto shutdown() -> void;

    [[nodiscard]] extern auto cfg() noexcept -> const assetmgr_config&;
    extern auto use_primary_accessor(const std::function<auto (asset_accessor& acc) -> void>& callback) -> void;
    [[nodiscard]] extern auto get_asset_request_count() noexcept -> std::size_t;
    [[nodiscard]] extern auto get_total_bytes_loaded() noexcept -> std::size_t;
}
