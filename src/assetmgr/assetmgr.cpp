// Copyright (c) 2022 Mario "Neo" Sieg. All Rights Reserved.

#include "assetmgr.hpp"
#include "../core/core.hpp"
#include "simdutf.h"

#include <atomic>
#include <fstream>

namespace assetmgr {
    using namespace std::filesystem;

    auto get_asset_root() -> const std::string& {
        static const std::string asset_root = ("/Users/mario/Documents/lunam/assets");
        return asset_root;
    }

    static constexpr std::string_view k_asset_type_names[] = {
        "icon",
        "mesh",
        "script",
        "shader",
        "texture",
        "material"
    };
    static_assert(std::size(k_asset_type_names) == static_cast<std::size_t>(asset_category::$count));

    static constexpr std::string_view k_asset_dir_names[] = {
        "icons",
        "meshes",
        "scripts",
        "shaders",
        "textures",
        "materials"
    };
    static_assert(std::size(k_asset_dir_names) == static_cast<std::size_t>(asset_category::$count));

    static constexpr char k_dir_sep = '/';

    static const std::array<std::string, static_cast<std::size_t>(asset_category::$count)> k_asset_dirs = [] {
        std::array<std::string, static_cast<std::size_t>(asset_category::$count)> dirs;
        for (std::size_t i = 0; i < dirs.size(); ++i) {
            dirs[i] = get_asset_root();
            dirs[i].push_back(k_dir_sep);
            dirs[i].append(k_asset_dir_names[i]);
        }
        return dirs;
    }();

    [[nodiscard]] static auto is_fs_valid() -> bool {
        if (!exists(get_asset_root())) [[unlikely]] {
            log_error("Asset directory not found: {}", get_asset_root());
            return false;
        }
        for (const std::string& dir : k_asset_dirs) {
            if (!exists(dir)) [[unlikely]] {
                log_error("Asset directory not found: {}", dir);
                return false;
            }
        }
        return true;
    }

    auto get_asset_dir(const asset_category category) -> const std::string& {
        return k_asset_dirs[static_cast<std::size_t>(category)];
    }

    auto get_asset_path(const asset_category category, const std::string& name) -> std::string {
        std::string dir = get_asset_dir(category);
        dir.push_back(k_dir_sep);
        dir.append(name);
        return dir;
    }

    static constinit std::atomic_uint64_t s_asset_requests = 0;
    static constinit std::atomic_uint64_t s_total_bytes_loaded = 0;

    [[nodiscard]] static auto validate_path(const std::string& full_path) -> bool {
        const int encoding = simdutf::autodetect_encoding(full_path.c_str(), full_path.size());
        if (encoding != simdutf::encoding_type::UTF8) [[unlikely]] { /* UTF8 != ASCII but UTF8 can store ASCII */
            log_warn("Asset path {} is not ASCII encoded", full_path);
            return false;
        }
        if (!simdutf::validate_ascii(full_path.c_str(), full_path.size())) [[unlikely]] {
            log_warn("Asset path {} contains non-ASCII characters", full_path);
            return false;
        }
        if (!exists(full_path)) [[unlikely]] {
            log_warn("Asset path not found: {}", full_path);
            return false;
        }
        return true;
    }

    template <typename T>
    [[nodiscard]] static auto load_asset(const std::string& path, T& out) -> bool {
        static constinit std::atomic_bool is_fs_validated = false;
        if (!is_fs_validated.load(std::memory_order_relaxed)) {
            if (!is_fs_valid()) [[unlikely]] {
                panic("Corrupted installation - asset filesystem is invalid - please reinstall");
            }
            is_fs_validated.store(true, std::memory_order_relaxed);
        }
        log_info("Asset load request #{}: {}", s_asset_requests.fetch_add(1, std::memory_order_relaxed), path);
        std::string abs = absolute(path).string();
        if (std::ranges::find(abs, '\\') != abs.cend()) { // Windows path
            std::ranges::replace(abs, '\\', '/'); // Convert to Unix path
        }
        if (!validate_path(abs)) [[unlikely]] {
            return false;
        }
        std::ifstream file {abs, std::ios::binary | std::ios::ate | std::ios::in};
        if (!file.is_open() || !file.good()) [[unlikely]] {
            log_error("Failed to open asset: {}", path);
            return false;
        }
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        out.resize(size);
        file.read(reinterpret_cast<char*>(out.data()), size);
        s_total_bytes_loaded.fetch_add(static_cast<std::uint64_t>(size), std::memory_order_relaxed);
        return true;
    }

    auto load_asset_blob_raw(const std::string& path, std::vector<std::uint8_t>& out) -> bool {
        return load_asset(path, out);
    }

    auto load_asset_text_raw(const std::string& path, std::string& out) -> bool {
        return load_asset(path, out);
    }

    auto load_asset_blob(const asset_category category, const std::string& name, std::vector<std::uint8_t>& out) -> bool {
        const std::string path = get_asset_path(category, name);
        return load_asset(path, out);
    }

    auto load_asset_text(asset_category category, const std::string& name, std::string& out) -> bool {
        const std::string path = get_asset_path(category, name);
        return load_asset(path, out);
    }

    auto get_asset_request_count() noexcept -> std::uint64_t {
        return s_asset_requests.load(std::memory_order_relaxed);
    }

    auto get_total_bytes_loaded() noexcept -> std::uint64_t {
        return s_total_bytes_loaded.load(std::memory_order_relaxed);
    }
}
