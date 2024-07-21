// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "stream.hpp"
#include "assetmgr.hpp"

#include <filesystem>

namespace lu::assetmgr {

    extern mgr_config s_cfg;
    extern constinit std::atomic_size_t s_asset_requests;
    extern constinit std::atomic_size_t s_asset_requests_failed;
    extern constinit std::atomic_size_t s_total_bytes_loaded;

    auto istream::read_all_bytes(std::vector<std::uint8_t>& out) -> bool {
        out.resize(static_cast<std::size_t>(get_length()));
        return read(out.data(), static_cast<std::streamsize>(out.size())) == out.size();
    }
    auto istream::read_all_bytes(std::string& out) -> bool {
        out.resize(static_cast<std::size_t>(get_length()));
        return read(out.data(), static_cast<std::streamsize>(out.size())) == out.size();
    }

    auto file_stream::open(std::string&& path) -> std::shared_ptr<file_stream> {
        std::string abs = std::filesystem::absolute(path).string();
        log_info("Asset stream request #{}: {}", s_asset_requests.fetch_add(1, std::memory_order_relaxed), abs);
        if (s_cfg.validate_paths && !validate_path(abs)) [[unlikely]] {
            s_asset_requests_failed.fetch_add(1, std::memory_order_relaxed);
            return nullptr;
        }
        class proxy : public file_stream {};
        auto stream = std::make_shared<proxy>();
        std::ifstream& file = stream->m_file;
        file.open(abs, std::ios::binary|std::ios::ate|std::ios::in);
        if (!file.is_open() || !file.good()) [[unlikely]] {
            s_asset_requests_failed.fetch_add(1, std::memory_order_relaxed);
            log_error("Failed to open file stream: {}", path);
            return nullptr;
        }
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        stream->m_size = size;
        stream->m_path = std::move(abs);
        s_total_bytes_loaded.fetch_add(size, std::memory_order_relaxed);
        return stream;
    }

    auto file_stream::set_pos(const std::streamsize pos, const std::ios::seekdir dir) -> bool {
        m_file.seekg(pos, dir);
        return m_file.good();
    }

    auto file_stream::get_pos() -> std::streamsize {
        return m_file.tellg();
    }

    auto file_stream::get_length() const -> std::streamsize {
        return m_size;
    }

    auto file_stream::read(void* buffer, const std::streamsize size) -> std::streamsize {
        m_file.read(static_cast<char*>(buffer), size);
        return m_file.good() ? m_file.gcount() : 0;
    }

    auto file_stream::get_path() const -> const std::string& {
        return m_path;
    }
}
