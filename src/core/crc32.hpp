// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>

namespace lu {
    // CRC32 implementation based on the polynomial 0xEDB88320. Ensure buffer is initialized to some data.
    [[nodiscard]] extern auto crc32(const void* buf, std::size_t len, std::uint32_t crc = 0) noexcept -> std::uint32_t;
}
