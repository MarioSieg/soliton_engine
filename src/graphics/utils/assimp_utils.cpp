// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "assimp_utils.hpp"

#include "../../core/core.hpp"
#include "../../assetmgr/assetmgr.hpp"

#include <filesystem>

namespace soliton::graphics {
    auto assimp_logger::write(const char* const message) -> void {
        const auto len = std::strlen(message);
        auto* copy = static_cast<char*>(alloca(len));
        std::memcpy(copy, message, len);
        copy[len - 1] = '\0'; // replace \n with \0
        log_info("[Mesh Import]: {}", copy);
    }

    lunam_io_stream::lunam_io_stream(eastl::vector<std::byte>&& stream) : m_stream{std::move(stream)} {}

    auto lunam_io_stream::Read(void* const buf, const std::size_t len, const std::size_t count) -> std::size_t {
        const std::size_t bytes_to_read = len * count;
        const std::size_t bytes_left = m_stream.size() - pos;
        const std::size_t bytes_read = std::min(bytes_to_read, bytes_left);
        std::memcpy(buf, m_stream.data() + pos, bytes_read);
        pos += bytes_read;
        return bytes_read / len;
    }

    auto lunam_io_stream::Write(const void* const buf, const std::size_t len, const std::size_t count) -> std::size_t {
        panic("Write not supported");
    }

    auto lunam_io_stream::Seek(const std::size_t offset, const aiOrigin origin) -> aiReturn {
        switch (origin) {
            case aiOrigin_SET:
                if (offset < m_stream.size()) [[likely]] {
                    pos = offset;
                    return aiReturn_SUCCESS;
                }
                return aiReturn_FAILURE;
            case aiOrigin_CUR:
                if (pos + offset < m_stream.size()) [[likely]] {
                    pos += offset;
                    return aiReturn_SUCCESS;
                }
                return aiReturn_FAILURE;
            case aiOrigin_END:
                if (offset < m_stream.size()) [[likely]] {
                    pos = m_stream.size() - offset;
                    return aiReturn_SUCCESS;
                }
                return aiReturn_FAILURE;
            default:
                panic("Unknown origin");
        }
    }

    auto lunam_io_stream::Tell() const -> std::size_t {
        return pos;
    }

    auto lunam_io_stream::FileSize() const -> std::size_t {
        return m_stream.size();
    }

    auto lunam_io_stream::Flush() -> void {
        panic("Flush not supported");
    }

    auto lunam_assimp_io_system::Exists(const char* const file) const -> bool {
        bool exists = false;
        eastl::string abs_path = "/";
        abs_path += std::filesystem::relative(file).string().c_str();
        assetmgr::with_primary_accessor_lock([&](assetmgr::asset_accessor& accessor) {
            exists = accessor.file_exists(abs_path.c_str());
        });
        return exists;
    }

    auto lunam_assimp_io_system::Open(const char* const file, const char* const mode) -> Assimp::IOStream* {
        eastl::vector<std::byte> data {};
        bool status = false;
        eastl::string abs_path = "/";
        abs_path += std::filesystem::relative(file).string().c_str();
        assetmgr::with_primary_accessor_lock([&](assetmgr::asset_accessor& accessor) {
            status = accessor.load_bin_file(abs_path.c_str(), data);
        });
        if (!status) [[unlikely]] {
            log_error("Failed to open file '{}'", abs_path);
        }
        return status ? new lunam_io_stream{std::move(data)} : nullptr;
    }

    auto lunam_assimp_io_system::Close(Assimp::IOStream* const f) -> void {
        delete f;
    }
}
