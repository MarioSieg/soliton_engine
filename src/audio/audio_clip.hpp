// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../assetmgr/assetmgr.hpp"

#include <fmod.hpp>

namespace soliton::audio {
    struct audio_clip_descriptor final {
        bool enable_3d = true;
        bool enable_looping = true;
        bool enable_streaming = false;
    };

    class audio_clip : public assetmgr::asset {
    public:
        explicit audio_clip(eastl::string&& asset_path,const audio_clip_descriptor& desc = {});
        explicit audio_clip(eastl::span<const std::byte> buf, const audio_clip_descriptor& desc);
        virtual ~audio_clip() override;

        [[nodiscard]] auto get_sound() const -> FMOD::Sound* { return m_sound; }

    protected:
        auto create_from_memory(eastl::span<const std::byte> buf, const audio_clip_descriptor& desc) -> void;

        FMOD::Sound* m_sound = nullptr;
    };
}
