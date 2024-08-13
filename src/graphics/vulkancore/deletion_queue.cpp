// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "deletion_queue.hpp"

namespace lu::vkb {
    deletion_queue::deletion_queue() {
        m_deleters.reserve(512);
    }

    auto deletion_queue::flush() -> void {
        // invoke in reverse order
        for (auto it = m_deleters.rbegin(); it != m_deleters.rend(); ++it) {
            (*it)();
        }
        m_deleters.clear();
    }
}
