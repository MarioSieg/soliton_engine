// Copyright (c) 2022 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <array>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>

#include <uuid.h>
#include <ankerl/unordered_dense.h>

#include "../core/core.hpp"

namespace lu::assetmgr {
    enum class asset_source : std::uint8_t {
        filesystem, // asset loaded from regular file from disk
        memory, // asset loaded from memory buffer
        package // asset loaded from compressed lunam package from disk
    };

    class asset : public no_copy, public no_move {
    protected:
        explicit asset(
            asset_source source,
            std::string&& asset_path = {}
        ) noexcept;

    public:
        virtual ~asset() = default;

        [[nodiscard]] auto get_uuid() const noexcept -> const uuids::uuid& { return m_uuid; }
        [[nodiscard]] auto get_source() const noexcept -> asset_source { return m_source; }
        [[nodiscard]] auto get_asset_path() const noexcept -> const std::string& { return m_asset_path; }
        [[nodiscard]] auto get_approx_byte_size() const noexcept -> std::size_t { return m_approx_byte_size; }

    private:
        const uuids::uuid m_uuid;
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

    class istream {
    public:
        virtual ~istream() = default;
        [[nodiscard]] virtual auto set_pos(std::streamsize pos, std::ios::seekdir dir) -> bool = 0;
        [[nodiscard]] virtual auto get_pos() -> std::streamsize = 0;
        [[nodiscard]] virtual auto get_length() const -> std::streamsize = 0;
        [[nodiscard]] virtual auto read(void* buffer, std::streamsize size) -> std::streamsize = 0;
        [[nodiscard]] virtual auto get_path() const -> const std::string& = 0;
        [[nodiscard]] auto read_all_bytes(std::vector<std::uint8_t>& out) -> bool;
        [[nodiscard]] auto read_all_bytes(std::string& out) -> bool;

    protected:
        istream() = default;
    };

    class file_stream : public istream {
    public:
        [[nodiscard]] static auto open(std::string&& path) -> std::shared_ptr<file_stream>;
        ~file_stream() override = default;
        [[nodiscard]] auto set_pos(std::streamsize pos, std::ios::seekdir dir) -> bool override;
        [[nodiscard]] auto get_pos() -> std::streamsize override;
        [[nodiscard]] auto get_length() const -> std::streamsize override;
        [[nodiscard]] auto read(void* buffer, std::streamsize size) -> std::streamsize override;
        [[nodiscard]] auto get_path() const -> const std::string& override;

    protected:
        explicit file_stream() = default;

        std::string m_path {};
        std::streamsize m_size {};
        std::ifstream m_file {};
    };

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
