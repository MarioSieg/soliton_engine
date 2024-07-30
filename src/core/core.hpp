// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#ifndef LUNAM_CORE_HPP
#define LUNAM_CORE_HPP

#include <EASTL/algorithm.h>
#include <EASTL/array.h>
#include <EASTL/bit.h>
#include <EASTL/fixed_vector.h>
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
#include <EASTL/shared_ptr.h>
#include <EASTL/weak_ptr.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/chrono.h>
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
#include "lazy.hpp"
#include "lazy_flex.hpp"
#include "move_copy_base.hpp"
#include "platform.hpp"
#include "spsc_queue.hpp"
#include "stopwatch.hpp"
#include "thread_signal.hpp"
#include "utils.hpp"

#include "specializations.hpp"

#define USE_MIMALLOC 1

namespace lu {
    [[nodiscard]] consteval auto make_version(const std::uint8_t major, const std::uint8_t minor) -> std::uint32_t { return (static_cast<std::uint32_t>(major)<<8)|minor; }
    [[nodiscard]] consteval auto major_version(const std::uint32_t v) -> std::uint8_t { return (v>>8)&0xff; }
    [[nodiscard]] consteval auto minor_version(const std::uint32_t v) -> std::uint8_t { return v&0xff; }

    constexpr std::uint32_t k_lunam_engine_v = make_version(0, 3); // current engine version (must be known at compile time and we don't use patches yet)
}

#endif
