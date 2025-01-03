// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/core.hpp"

#include <efsw/efsw.hpp>

namespace soliton {
    using watch_id = efsw::WatchID;

    class fs_watchdog : public efsw::FileWatchListener {
    public:
        fs_watchdog();
        virtual ~fs_watchdog() = default;

        auto add_watch(const eastl::string& path, bool recursive) -> watch_id;
        auto remove_watch(watch_id id) -> void;
        auto start_watching_async() -> void;

        using event = eastl::function<auto(eastl::string&& dir, eastl::string&& filename, eastl::string&& old_filename) -> void>;
        multicast_delegate<event> on_any_event {};
        multicast_delegate<event> on_add {};
        multicast_delegate<event> on_delete {};
        multicast_delegate<event> on_modify {};
        multicast_delegate<event> on_move {};

    private:
        eastl::unique_ptr<efsw::FileWatcher> m_watcher {};

        auto handleFileAction(
            efsw::WatchID id,
            const std::string& dir,
            const std::string& filename,
            efsw::Action action,
            std::string old_filename
        ) -> void override;
    };
}
