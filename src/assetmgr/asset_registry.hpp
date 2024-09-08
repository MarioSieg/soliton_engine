// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <ankerl/unordered_dense.h>

#include "asset.hpp"

namespace lu::assetmgr {
    enum class asset_ref : std::int32_t {}; // Asset reference type to access asset instances by index (used from Lua)
    static constexpr asset_ref asset_ref_invalid = static_cast<asset_ref>(std::numeric_limits<std::underlying_type_t<asset_ref>>::max());
    static_assert(0x7fffffff == static_cast<std::size_t>(asset_ref_invalid)); // Invalid asset ref constant also used in Lua

    // Maximum number of valid asset references
    static constexpr asset_ref asset_ref_max = static_cast<asset_ref>(static_cast<std::size_t>(asset_ref_invalid) - 1);

    template <typename  T> requires is_asset<T>
    class asset_registry final {
    public:
        explicit asset_registry(std::size_t capacity = 32) {
            m_loaded.reserve(capacity);
            m_path_cache.reserve(capacity);
        }

        [[nodiscard]] auto insert(eastl::unique_ptr<T>&& asset = nullptr, const eastl::string_view path = {}) -> eastl::pair<asset_ref, T*> {
            if constexpr (std::is_constructible_v<T>) {
                if (!asset) {
                    asset = eastl::make_unique<T>();
                }
            }
            passert(asset != nullptr);
            if (!path.empty()) { // Check if asset with path already exists
                const auto it = m_path_cache.find(path);
                if (it != m_path_cache.end()) { // Return existing asset reference
                    T* const loaded = (*this)[it->second];
                    passert(loaded != nullptr);
                    return eastl::make_pair(it->second, loaded);
                }
            }
            const std::size_t idx = m_loaded.size();
            if (idx > static_cast<std::size_t>(asset_ref_max)) [[unlikely]] {
                log_error("Asset asset_ref numeric capacity exhausted, cannot insert asset: {}", path);
                return eastl::make_pair(asset_ref_invalid, nullptr);
            }
            const auto ref = static_cast<asset_ref>(idx);
            T* const ptr = &*asset;
            m_loaded.emplace_back(eastl::move(asset));
            if (!path.empty()) {
                m_path_cache.emplace(path, ref);
            }
            return eastl::make_pair(ref, ptr);
        }

        [[nodiscard]] auto load(eastl::string&& path) -> asset_ref {
            if (path.empty()) {
                log_error("Cannot load asset with empty path");
                return asset_ref_invalid;
            }
            const auto it = m_path_cache.find(path);
            if (it != m_path_cache.end()) {
                return it->second;
            }
            eastl::string_view path_view { path };
            auto asset = eastl::make_unique<T>(eastl::move(path));
            if (!asset) {
                log_error("Failed to load asset from path: {}", path_view);
                return asset_ref_invalid;
            }
            return insert(eastl::move(asset), path_view).first;
        }

        auto print_loaded() const -> void {
            log_info("---- Registered {} Assets -----", m_loaded.size());
            for (std::size_t i = 0; i < m_loaded.size(); ++i) {
                log_info("Asset[{}]: {}", i, m_loaded[i]->get_asset_path());
            }
        }

        [[nodiscard]] auto operator[](const asset_ref ref) const -> T* {
            if (ref == asset_ref_invalid || static_cast<std::size_t>(ref) >= m_loaded.size()) [[unlikely]] {
                return nullptr;
            }
            if (ref > asset_ref_max) [[unlikely]] {
                log_error("Invalid asset reference: {}", static_cast<std::size_t>(ref));
                return nullptr;
            }
            return &*m_loaded[static_cast<std::size_t>(ref)];
        }

        [[nodiscard]] auto loaded() const -> const eastl::vector<eastl::unique_ptr<T>>& { return m_loaded; }

        [[nodiscard]] auto size() const -> std::size_t { return m_loaded.size(); }

        auto invalidate() -> void {
            m_path_cache.clear();
            m_loaded.clear();
        }

    private:
        eastl::vector<eastl::unique_ptr<T>> m_loaded {};
        ankerl::unordered_dense::map<eastl::string_view, asset_ref> m_path_cache {};
    };
}
