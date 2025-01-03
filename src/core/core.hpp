// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#ifndef LUNAM_CORE_HPP
#define LUNAM_CORE_HPP

#include <EASTL/algorithm.h>
#include <EASTL/array.h>
#include <EASTL/bit.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_string.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/span.h>
#include <EASTL/optional.h>
#include <EASTL/variant.h>
#include <EASTL/tuple.h>
#include <EASTL/functional.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/sort.h>
#include <EASTL/list.h>
#include <EASTL/stack.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/weak_ptr.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/chrono.h>
#include <EASTL/queue.h>
#include <EASTL/map.h>
#include <EASTL/initializer_list.h>
#include <EAStdC/EABitTricks.h>

#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>

#include <ankerl/unordered_dense.h>

#include "crc32.hpp"
#include "delegate.hpp"
#include "exit_guard.hpp"
#include "hash.hpp"
#include "move_copy_base.hpp"
#include "platform.hpp"
#include "stopwatch.hpp"
#include "thread_signal.hpp"
#include "utils.hpp"
#include "specializations.hpp"

#define USE_MIMALLOC 1

namespace soliton {
    [[nodiscard]] constexpr auto make_version(const std::uint8_t major, const std::uint8_t minor) -> std::uint32_t { return (static_cast<std::uint32_t>(major)<<8)|minor; }
    [[nodiscard]] constexpr auto major_version(const std::uint32_t v) -> std::uint8_t { return (v>>8)&0xff; }
    [[nodiscard]] constexpr auto minor_version(const std::uint32_t v) -> std::uint8_t { return v&0xff; }
    [[nodiscard]] constexpr auto unpack_version(const std::uint32_t v) -> eastl::array<std::uint8_t, 2> { return {major_version(v), minor_version(v)}; }

    constexpr std::uint32_t k_lunam_engine_version = make_version(0, 5); // current engine version (must be known at compile time and we don't use patches yet)

    [[nodiscard]] extern auto get_version_string() -> eastl::string;

    using namespace DirectX; // Re-export DirectX namespace
}

#endif
