// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <assimp/DefaultLogger.hpp>
#include <assimp/StringComparison.h>
#include <assimp/IOSystem.hpp>
#include <assimp/DefaultIOStream.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/LogStream.hpp>

namespace lu::graphics {
    class assimp_logger final : public Assimp::LogStream {
        auto write(const char* message) -> void override;
    };

    class lunam_io_stream final : public Assimp::IOStream {
    public:
        explicit lunam_io_stream(std::vector<std::byte>&& stream);
        auto Read(void* buf, std::size_t len, std::size_t count) -> std::size_t override;
        auto Write(const void* buf, size_t len, size_t count) -> std::size_t override;
        auto Seek(std::size_t offset, aiOrigin origin) -> aiReturn override;
        [[nodiscard]] auto Tell() const -> std::size_t override;
        [[nodiscard]] auto FileSize() const -> std::size_t override;
        auto Flush() -> void override;

    private:
        std::size_t pos = 0;
        const std::vector<std::byte> m_stream;
    };

    class lunam_assimp_io_system final : public Assimp::DefaultIOSystem {
    public:
        auto Exists(const char* file) const -> bool override;
        auto Open(const char* file, const char* mode) -> Assimp::IOStream* override;
        auto Close(Assimp::IOStream* f) -> void override;
    };

}