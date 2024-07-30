// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include <mimalloc.h>

auto operator new [](const std::size_t size, const std::size_t alignment, const std::size_t, const char*, int, unsigned, const char*, int) -> void* {
    return mi_malloc_aligned(size, alignment);
}
auto operator new [](const std::size_t size, const char*, int, unsigned, const char*, int) -> void* {
    return mi_malloc(size);
}