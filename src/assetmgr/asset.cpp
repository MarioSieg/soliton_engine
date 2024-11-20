// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "asset.hpp"

namespace soliton {
    thread_local std::mt19937 s_rng {std::random_device{}()};
    thread_local uuids::uuid_random_generator s_uuid_gen {s_rng};
}

namespace soliton::assetmgr {
    asset::asset(const asset_source source, eastl::string&& asset_path) noexcept
        : m_uuid{s_uuid_gen()}, m_source{source}, m_asset_path{std::move(asset_path)} {
    }

    asset::~asset() {

    }
}

