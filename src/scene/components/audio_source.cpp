// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "audio_source.hpp"
#include "../../audio/audio_subsystem.hpp"

namespace lu::com {
    auto audio_source::play() -> void {
        if (!clip) [[unlikely]] {
            log_warn("no audio clip to play");
            return;
        }
        FMOD::System* const system = audio::audio_subsystem::get_system();
        passert(system);
        fmod_check(system->playSound(clip->get_sound(), nullptr, false, &m_channel));
    }
}