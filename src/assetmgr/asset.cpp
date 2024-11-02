// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "asset.hpp"

namespace lu::assetmgr {
    thread_local std::mt19937 s_rng {std::random_device{}()};
    thread_local uuids::uuid_random_generator s_uuid_gen {s_rng};

    asset::asset(const asset_source source, eastl::string&& asset_path) noexcept
        : m_uuid{s_uuid_gen()}, m_source{source}, m_asset_path{std::move(asset_path)} {
        log_info("Asset '{}' created", m_asset_path.empty() ? uuids::to_string(m_uuid).c_str() : m_asset_path.c_str());
    }

    asset::~asset() {
        log_info("Asset '{}' destroyed", m_asset_path.empty() ? uuids::to_string(m_uuid).c_str() : m_asset_path.c_str());
    }
}

