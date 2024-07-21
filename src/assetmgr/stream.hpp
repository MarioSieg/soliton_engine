// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <cstdint>
#include <fstream>

namespace lu::assetmgr {
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
}
