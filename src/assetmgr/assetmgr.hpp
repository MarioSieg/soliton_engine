// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "asset.hpp"
#include "asset_registry.hpp"
#include "asset_accessor.hpp"

#include <uuid.h>

#include "../core/core.hpp"

namespace lu::assetmgr {
    // Virtual file system mounts: {physical path, virtual path}, physical path can either be a directory or a LUPACK archive
    constexpr eastl::array<eastl::pair<eastl::string_view, eastl::string_view>, 1> k_vfs_mounts {
        eastl::make_pair("./engine_assets", "/engine_assets")
    };

    extern auto init() -> void;
    extern auto shutdown() -> void;

    extern auto with_primary_accessor_lock(const eastl::function<auto (asset_accessor& acc) -> void>& callback) -> void;
    [[nodiscard]] extern auto get_asset_request_count() noexcept -> std::size_t;
    [[nodiscard]] extern auto get_total_bytes_loaded() noexcept -> std::size_t;
}
