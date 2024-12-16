#pragma once

#include <cstdint>

namespace engine
{
    // CPU cache line size, set 64 bytes here.
    constexpr std::size_t CPU_CACHELINE_SIZE = 64; // Maybe 32 bytes, 64 bytes, 128 bytes
}