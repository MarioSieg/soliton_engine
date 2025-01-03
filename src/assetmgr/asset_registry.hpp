// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <ankerl/unordered_dense.h>

#include "asset.hpp"

namespace soliton::assetmgr {
    using interop_asset_id = std::uint32_t; /* Asset ID represented in LUA */
    constexpr interop_asset_id invalid_asset_id = 0x7fffffffu;

    template <typename T>
    class asset_registry final {
    public:
        explicit asset_registry(std::size_t capacity = 32) {
            m_loaded.reserve(capacity);
        }

        [[nodiscard]] auto insert(eastl::unique_ptr<T>&& asset = nullptr, eastl::string_view path = {}) -> T* {
            if constexpr (std::is_constructible_v<T>) {
                if (!asset) {
                    asset = eastl::make_unique<T>();
                }
            }
            panic_assert(asset != nullptr);
            if (path.empty()) {
                const uuids::uuid& uuid = asset->get_uuid();
                const_cast<eastl::string&>(asset->get_asset_path()) // Safe because underlying string is not const
                    = to_string(uuid).c_str(); // Use UUID as path
                path = asset->get_asset_path();
            }
            if (const auto it = m_loaded.find(path); it != m_loaded.end()) {
                T* const loaded = &*it->second;
                panic_assert(loaded != nullptr);
                return loaded;
            }
            T* p = &*asset;
            m_loaded[path] = eastl::move(asset);
            return p;
        }

        template <typename... Args>
        [[nodiscard]] auto load(eastl::string&& path, Args&&... args) -> T* {
            if (path.empty()) {
                log_error("Cannot load asset with empty path");
                return nullptr;
            }
            if (const auto it = m_loaded.find(path); it != m_loaded.end()) {
                return &*it->second;
            }
            eastl::string_view path_view { path };
            auto asset = eastl::make_unique<T>(eastl::move(path), eastl::forward<Args>(args)...);
            if (!asset) {
                log_error("Failed to load asset from path: {}", path_view);
                return nullptr;
            }
            return insert(eastl::move(asset), path_view);
        }

        auto print_loaded() const -> void {
            log_info("---- Registered {} Assets -----", m_loaded.size());
            for (std::size_t i = 0; i < m_loaded.size(); ++i) {
                log_info("Asset[{}]: {}", i, m_loaded[i]->get_asset_path());
            }
        }

        [[nodiscard]] auto operator[](const eastl::string_view path) const -> T* {
            if (const auto it = m_loaded.find(path); it != m_loaded.end()) {
                return &*m_loaded[it->second];
            }
            return nullptr;
        }

        [[nodiscard]] auto loaded() const -> const ankerl::unordered_dense::map<eastl::string_view, eastl::unique_ptr<T>>& { return m_loaded; }

        auto invalidate() -> void {
            m_loaded.clear();
        }

        // Lua interop helpers
        class lua_interop final {
        public:
            explicit constexpr lua_interop(asset_registry& host) : host{host} {}

            template <typename... Args>
            [[nodiscard]] auto load(eastl::string&& path, Args&&... args) -> interop_asset_id {
                T* asset = host.load(eastl::move(path), eastl::forward<Args>(args)...);
                if (!asset) {
                    return invalid_asset_id;
                }
                std::size_t id = all.size();
                panic_assert(id < invalid_asset_id);
                all.emplace_back(asset);
                return static_cast<interop_asset_id>(id);
            }

            [[nodiscard]] auto operator[](const interop_asset_id id) const -> T* {
                if (id >= all.size()) {
                    log_warn("Asset ID {} is out of bounds", id);
                    return nullptr;
                }
                return all[id];
            }

        private:
            eastl::vector<T*> all {};
            asset_registry& host;
        };
        lua_interop interop {*this};
    private:
        ankerl::unordered_dense::map<eastl::string_view, eastl::unique_ptr<T>> m_loaded {};
    };
}
