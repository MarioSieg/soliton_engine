// Copyright (c) 2022 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <array>
#include <string>
#include <filesystem>
#include <vector>

#include "../core/core.hpp"

enum class asset_category {
    icon,
    mesh,
    script,
    shader,
    texture,

    $count
};

namespace assetmgr {
    [[nodiscard]] extern auto get_asset_root() -> const std::string&;
    [[nodiscard]] extern auto get_asset_dir(asset_category category) -> const std::string&;
    [[nodiscard]] extern auto get_asset_path(asset_category category, const std::string& name) -> std::string;
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
}
