// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/subsystem.hpp"

#include <fmod.hpp>
#include <fmod_errors.h>

namespace lu::audio {
    #define fmod_check(f) \
        if (const FMOD_RESULT rrr = (f); rrr != FMOD_OK) [[unlikely]] { \
            panic("Audio engine error: {} -> " #f, FMOD_ErrorString(rrr)); \
        }

    class audio_subsystem final : public subsystem {
    public:
        audio_subsystem();
        ~audio_subsystem() override;

    private:
        virtual auto on_post_tick() -> void override;

        FMOD::System* m_system = nullptr;
    };
}
