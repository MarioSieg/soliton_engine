// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <uuid.h>

#include "../core/core.hpp"

namespace soliton::assetmgr {
    enum class asset_source : std::uint8_t {
        filesystem, // asset loaded from regular file from disk
        memory, // asset loaded from memory buffer
        package // asset loaded from compressed soliton_engine package from disk
    };

    class asset : public no_copy, public no_move {
    protected:
        explicit asset(
            asset_source source,
            eastl::string&& asset_path = {}
        ) noexcept;

    public:
        virtual ~asset();

        [[nodiscard]] auto get_uuid() const noexcept -> const uuids::uuid& { return m_uuid; }
        [[nodiscard]] auto get_source() const noexcept -> asset_source { return m_source; }
        [[nodiscard]] auto get_asset_path() const noexcept -> const eastl::string& { return m_asset_path; }
        [[nodiscard]] auto get_approx_byte_size() const noexcept -> std::size_t { return m_approx_byte_size; }

    private:
        const uuids::uuid m_uuid;
        const asset_source m_source;
        const eastl::string m_asset_path;

    protected:
        std::size_t m_approx_byte_size = 0;
    };

    template<typename T>
    concept is_asset = std::is_base_of_v<asset, T>;
}
