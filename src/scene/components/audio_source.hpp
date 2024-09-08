// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../base.hpp"

#include "../../audio/audio_clip.hpp"

namespace lu::com {
    struct audio_source final {
    public:
        audio::audio_clip* clip = nullptr;

        auto play() -> void;

    private:
        FMOD::Channel* m_channel = nullptr;
    };
}
