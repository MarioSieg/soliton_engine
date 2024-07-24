// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "asset_accessor.hpp"
#include "assetmgr.hpp"

#include <simdutf.h>
#include <mimalloc.h>
#define STRPOOL_IMPLEMENTATION
#define STRPOOL_MALLOC(ctx, size) (mi_malloc(size))
#define STRPOOL_FREE(ctx, ptr) (mi_free(ptr))
#include <strpool.h>
#define ASSETSYS_IMPLEMENTATION
#define ASSETSYS_MALLOC(ctx, size) (mi_malloc(size))
#define ASSETSYS_FREE(ctx, ptr) (mi_free(ptr))
#include <assetsys.h>

namespace lu::assetmgr {
    extern constinit std::atomic_size_t s_asset_requests;
    extern constinit std::atomic_size_t s_asset_requests_failed;
    extern constinit std::atomic_size_t s_total_bytes_loaded;

    [[nodiscard]] static constexpr auto asset_sys_err_info(const assetsys_error_t error) noexcept -> const char* {
        switch (error) {
            case ASSETSYS_SUCCESS:
                return "Success";
            case ASSETSYS_ERROR_INVALID_PATH:
                return "Invalid path";
            case ASSETSYS_ERROR_INVALID_MOUNT:
                return "Invalid mount";
            case ASSETSYS_ERROR_FAILED_TO_READ_ZIP:
                return "Failed to read ZIP";
            case ASSETSYS_ERROR_FAILED_TO_CLOSE_ZIP:
                return "Failed to close ZIP";
            case ASSETSYS_ERROR_FAILED_TO_READ_FILE:
                return "Failed to read file";
            case ASSETSYS_ERROR_FILE_NOT_FOUND:
                return "File not found";
            case ASSETSYS_ERROR_DIR_NOT_FOUND:
                return "Directory not found";
            case ASSETSYS_ERROR_INVALID_PARAMETER:
                return "Invalid parameter";
            case ASSETSYS_ERROR_BUFFER_TOO_SMALL:
                return "Buffer too small";
            default:
                return "Unknown error";
        }
    }

    asset_accessor::asset_accessor() : m_id{s_accessor_id_gen.fetch_add(1, std::memory_order_relaxed)} {
        log_info("Creating asset accessor {}", m_id);
        m_sys = assetsys_create(nullptr);
        passert(m_sys != nullptr);
        for (const auto [fs, vfs] : k_vfs_mounts) {
            log_info("Mounting asset root '{}' -> '{}", fs, vfs);
            if (assetsys_error_t err = assetsys_mount(m_sys, fs.data(), vfs.data()); err != ASSETSYS_SUCCESS) { // Attempt to mount dir first
                const std::string lupack_file = fmt::format("{}.lupack", fs);
                if (err = assetsys_mount(m_sys, lupack_file.c_str(), vfs.data()); err != ASSETSYS_SUCCESS) [[unlikely]] { // Attempt to mount LUPACK file now
                    panic("Failed to mount asset root (PFS or VFS) '{}' / '{} to '{}: {}", fs, lupack_file, vfs, asset_sys_err_info(err)); // Panic if both failed
                }
            }
        }
        s_accessors_online.fetch_add(1, std::memory_order_relaxed);
    }

    asset_accessor::~asset_accessor() {
        log_info("Destroying asset accessor {}", m_id);
        log_info(
            "Collated: {}, Mounts: {}, Requests: {}, Failed Requests: {}, Total Loaded: {:.03f} MiB",
            m_sys->collated_count,
            m_sys->mounts_count,
            m_num_request,
            m_num_failed_requests,
            static_cast<double>(m_total_bytes_loaded) / static_cast<double>(1<<20)
        );
        assetsys_destroy(m_sys);
        s_accessors_online.fetch_sub(1, std::memory_order_relaxed);
    }

    auto asset_accessor::file_count(const char* const vpath) const noexcept -> std::size_t {
        return assetsys_file_count(m_sys, vpath);
    }
    auto asset_accessor::file_name(const char* const vpath, const std::size_t idx) const noexcept -> const char* {
        return assetsys_file_name(m_sys, vpath, static_cast<int>(idx));
    }
    auto asset_accessor::file_path(const char* const vpath, const std::size_t idx) const noexcept -> const char* {
        return assetsys_file_path(m_sys, vpath, static_cast<int>(idx));
    }
    auto asset_accessor::dir_count(const char* const vpath) const noexcept -> std::size_t {
        return assetsys_subdir_count(m_sys, vpath);
    }
    auto asset_accessor::dir_name(const char* const vpath, const std::size_t idx) const noexcept -> const char* {
        return assetsys_subdir_name(m_sys, vpath, static_cast<int>(idx));
    }
    auto asset_accessor::dir_path(const char* const vpath, const std::size_t idx) const noexcept -> const char* {
        return assetsys_subdir_name(m_sys, vpath, static_cast<int>(idx));
    }
    auto asset_accessor::mount(const char* const rpath, const char* const vpath) const -> bool {
        return assetsys_mount(m_sys, rpath, vpath);
    }
    auto asset_accessor::unmount(const char* const rpath, const char* const vpath) const -> void {
        assetsys_dismount(m_sys, rpath, vpath);
    }

    template <typename T>
    [[nodiscard]] static auto load_file_impl(
        assetsys_t *const m_sys,
        const char* const vpath,
        T& dat,
        std::uint32_t& num_requests,
        std::uint32_t& num_failed_requests
    ) -> bool {
        ++num_requests;
        s_asset_requests.fetch_add(1, std::memory_order_relaxed);
        const simdutf::encoding_type encoding = simdutf::autodetect_encoding(vpath, std::strlen(vpath));
        if (encoding != simdutf::encoding_type::UTF8 && encoding != simdutf::encoding_type::unspecified) [[unlikely]] {
            log_error("File '{}' is not UTF-8 encoded", vpath);
            s_asset_requests_failed.fetch_add(1, std::memory_order_relaxed);
            ++num_failed_requests;
            return false;
        }
        assetsys_file_t info {};
        if (const assetsys_error_t err = assetsys_file(m_sys, vpath, &info); err != ASSETSYS_SUCCESS) [[unlikely]] {
            log_error("Failed to load file '{}': {}", vpath, asset_sys_err_info(err));
            s_asset_requests_failed.fetch_add(1, std::memory_order_relaxed);
            ++num_failed_requests;
            return false;
        }
        const int size = assetsys_file_size(m_sys, info);
        if (!size) [[unlikely]] {
            log_error("File '{}' is empty", vpath);
            s_asset_requests_failed.fetch_add(1, std::memory_order_relaxed);
            ++num_failed_requests;
            return false;
        }
        dat.clear();
        dat.resize(size);
        s_total_bytes_loaded.fetch_add(size, std::memory_order_relaxed);
        int loaded_size = 0;
        if (const assetsys_error_t err = assetsys_file_load(m_sys, info, &loaded_size, dat.data(), size);
                err != ASSETSYS_SUCCESS || loaded_size != size) [[unlikely]] {
            log_error("Failed to read file '{}': {}", vpath, asset_sys_err_info(err));
            s_asset_requests_failed.fetch_add(1, std::memory_order_relaxed);
            ++num_failed_requests;
            return false;
        }
        return true;
    }

    auto asset_accessor::load_bin_file(const char* const vpath, std::vector<std::byte>& dat) -> bool {
        const auto result = load_file_impl(m_sys, vpath, dat, m_num_request, m_num_failed_requests);
        m_total_bytes_loaded += dat.size();
        return result;
    }
    auto asset_accessor::load_txt_file(const char* const vpath, std::string& dat) -> bool {
        const auto result = load_file_impl(m_sys, vpath, dat, m_num_request, m_num_failed_requests);
        m_total_bytes_loaded += dat.size();
        return result;
    }

    auto asset_accessor::dump_dir_tree(const char* const vpath, const int indent) -> void {
        for (int i = 0; i < assetsys_subdir_count(m_sys, vpath ); ++i) {
            char const* subdir_name = assetsys_subdir_name(m_sys, vpath, i );
            for( int j = 0; j < indent; ++j ) std::printf( "  " );
            std::printf( "%s/\n", subdir_name );
            char const* subdir_path = assetsys_subdir_path(m_sys, vpath, i );
            dump_dir_tree(subdir_path, indent + 1);
        }
        for (int i = 0; i < assetsys_file_count(m_sys, vpath ); ++i){
            char const* file_name = assetsys_file_name(m_sys, vpath, i );
            for( int j = 0; j < indent; ++j ) std::printf( "  " );
            std::printf( "%s\n", file_name );
        }
    }
}
