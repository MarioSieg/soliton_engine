// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/subsystem.hpp"
#include "../scene/components.hpp"

#include "audio_clip.hpp"

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

        [[nodiscard]] static auto get_system() noexcept -> FMOD::System* { return m_system; }

    private:
        virtual auto on_post_tick() -> void override;
        auto set_audio_listener_transform(const com::transform& transform) noexcept -> void;

        static inline constinit FMOD::System* m_system = nullptr;
    };
}
