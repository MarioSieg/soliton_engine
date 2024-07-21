// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <assimp/DefaultLogger.hpp>
#include <assimp/StringComparison.h>
#include <assimp/IOSystem.hpp>
#include <assimp/DefaultIOStream.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/LogStream.hpp>

namespace lu::graphics {
    class lunam_io_stream : public Assimp::IOStream {
    public:
        explicit lunam_io_stream(std::shared_ptr<assetmgr::istream>&& stream) : m_stream{std::move(stream)} {
            passert(m_stream != nullptr);
        }

        auto Read(void* pvBuffer, std::size_t pSize, std::size_t pCount) -> std::size_t override {
            return m_stream->read(pvBuffer, static_cast<std::streamsize>(pSize * pCount)) / pSize;
        }

        auto Write(const void* pvBuffer, size_t pSize, size_t pCount) -> std::size_t override {
            panic("Write not supported");
        }

        auto Seek(size_t pOffset, aiOrigin pOrigin) -> aiReturn override {
            switch (pOrigin) {
                case aiOrigin_SET:
                    return m_stream->set_pos(static_cast<std::streamsize>(pOffset), std::ios::beg) ? aiReturn_SUCCESS
                                                                                                   : aiReturn_FAILURE;
                case aiOrigin_CUR:
                    return m_stream->set_pos(static_cast<std::streamsize>(pOffset), std::ios::cur) ? aiReturn_SUCCESS
                                                                                                   : aiReturn_FAILURE;
                case aiOrigin_END:
                    return m_stream->set_pos(static_cast<std::streamsize>(pOffset), std::ios::end) ? aiReturn_SUCCESS
                                                                                                   : aiReturn_FAILURE;
                default:
                    panic("Unknown origin");
            }
        }

        [[nodiscard]] auto Tell() const -> std::size_t override {
            return static_cast<std::size_t>(m_stream->get_pos());
        }

        [[nodiscard]] auto FileSize() const -> std::size_t override {
            return static_cast<std::size_t>(m_stream->get_length());
        }

        auto Flush() -> void override {
            panic("Flush not supported");
        }

    private:
        const std::shared_ptr<assetmgr::istream> m_stream;
    };

    class lunam_assimp_io_system : public Assimp::DefaultIOSystem {
    public:
        auto Open(const char* file, const char* mode) -> Assimp::IOStream* override {
            auto stream = assetmgr::file_stream::open(file);
            if (stream) [[likely]] {
                return new lunam_io_stream{std::move(stream)};
            } else {
                return nullptr;
            }
        }

        auto Close(Assimp::IOStream* f) -> void override {
            delete f;
        }
    };

    class assimp_logger final : public Assimp::LogStream {
        auto write(const char* message) -> void override {
            const auto len = std::strlen(message);
            auto* copy = static_cast<char*>(alloca(len));
            std::memcpy(copy, message, len);
            copy[len - 1] = '\0'; // replace \n with \0
            log_info("[Mesh Import]: {}", copy);
        }
    };
}
