// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "audio_subsystem.hpp"
#include "../scripting/system_variable.hpp"

namespace lu::audio {
    static const system_variable<std::uint32_t> sv_max_audio_channels {"audio.max_channels", {512}};

    audio_subsystem::audio_subsystem() : subsystem{"Audio"} {
        log_info("Initializing audio engine v.{:x}", FMOD_VERSION);
        fmod_check(FMOD::System_Create(&m_system, FMOD_VERSION));
        fmod_check(m_system->init(sv_max_audio_channels(), FMOD_INIT_NORMAL, nullptr));
    }

    audio_subsystem::~audio_subsystem() {
        log_info("Shutting down audio engine");
        fmod_check(m_system->close());
        fmod_check(m_system->release());
    }

    auto audio_subsystem::on_post_tick() -> void {
        fmod_check(m_system->update());
    }
}
