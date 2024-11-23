// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "fs_watchdog.hpp"

namespace soliton {
    fs_watchdog::fs_watchdog() {
        m_watcher = eastl::make_unique<efsw::FileWatcher>();
    }

    auto fs_watchdog::handleFileAction(
        efsw::WatchID id,
        const std::string& dir,
        const std::string& filename,
        efsw::Action action,
        std::string old_filename
    ) -> void {
        eastl::invoke(on_any_event, dir.c_str(), filename.c_str(), old_filename.c_str());
        switch ( action ) {
            case efsw::Actions::Add: eastl::invoke(on_add, dir.c_str(), filename.c_str(), old_filename.c_str()); return;
            case efsw::Actions::Delete: eastl::invoke(on_delete, dir.c_str(), filename.c_str(), old_filename.c_str()); return;
            case efsw::Actions::Modified: eastl::invoke(on_modify, dir.c_str(), filename.c_str(), old_filename.c_str()); return;
            case efsw::Actions::Moved: eastl::invoke(on_move, dir.c_str(), filename.c_str(), old_filename.c_str()); return;
            default: return;
        }
    }

    auto fs_watchdog::add_watch(const eastl::string& path, bool recursive) -> watch_id {
        log_info("Registered watching dir {}, recursive: {}", path, recursive);
        return m_watcher->addWatch(path.c_str(), this, recursive);
    }

    auto fs_watchdog::remove_watch(watch_id id) -> void {
        m_watcher->removeWatch(id);
    }

    auto fs_watchdog::start_watching_async() -> void {
        std::size_t watch_total = 0;
        watch_total += on_any_event.listeners().size();
        watch_total += on_add.listeners().size();
        watch_total += on_delete.listeners().size();
        watch_total += on_modify.listeners().size();
        watch_total += on_move.listeners().size();
        log_info("Starting async fs watching...");
        m_watcher->watch();
    }
}
