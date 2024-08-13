// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../../core/core.hpp"

namespace lu::vkb {
    class deletion_queue final {
    public:
        deletion_queue();
        inline auto push(eastl::function<auto() -> void>&& deleter) -> void {
            m_deleters.push_back(eastl::forward<decltype(deleter)>(deleter));
        }

        auto flush() -> void;

    private:
        eastl::vector<eastl::function<auto() -> void>> m_deleters {};
    };
}
