// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>

namespace lu {
    [[nodiscard]] extern auto crc32(const void* buffer, std::size_t size) noexcept -> std::uint32_t;
}
