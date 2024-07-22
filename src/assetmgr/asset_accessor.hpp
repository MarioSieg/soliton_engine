// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <cstddef>
#include <vector>

extern "C" struct assetsys_t;

namespace lu::assetmgr {
    // Context to read/write assets from within the virtual file system.
    // This class is not thread-safe, so each thread should have its own instance.
    class asset_accessor final {
    public:
        asset_accessor();
        asset_accessor(const asset_accessor&) = delete;
        asset_accessor(asset_accessor&&) = delete;
        auto operator = (const asset_accessor&) -> asset_accessor& = delete;
        auto operator = (asset_accessor&&) -> asset_accessor& = delete;
        ~asset_accessor();

        [[nodiscard]] auto sys() const noexcept -> assetsys_t* { return m_sys; }
        auto dump_dir_tree(const char* vpath, const int indent = 0) -> void;
        [[nodiscard]] auto file_count(const char* vpath) const noexcept -> std::size_t;
        [[nodiscard]] auto file_name(const char* vpath, std::size_t idx) const noexcept -> const char*;
        [[nodiscard]] auto file_path(const char* vpath, std::size_t idx) const noexcept -> const char*;
        [[nodiscard]] auto dir_count(const char* vpath) const noexcept -> std::size_t;
        [[nodiscard]] auto dir_name(const char* vpath, std::size_t idx) const noexcept -> const char*;
        [[nodiscard]] auto dir_path(const char* vpath, std::size_t idx) const noexcept -> const char*;
        [[nodiscard]] auto load_bin_file(const char* vpath, std::vector<std::byte>& dat) const -> bool;
        [[nodiscard]] auto load_txt_file(const char* vpath, std::string& dat) const -> bool;
        [[nodiscard]] auto mount(const char* rpath, const char* vpath) const -> bool;
        auto unmount(const char* rpath, const char* vpath) const -> void;

        [[nodiscard]] static auto accessors_online() noexcept -> std::uint32_t { return s_accessors_online.load(std::memory_order_relaxed); }

    private:
        const std::uint32_t m_id;
        static inline constinit std::atomic_uint32_t s_accessors_online, s_accessor_id_gen;
        assetsys_t* m_sys = nullptr;
    };
}
