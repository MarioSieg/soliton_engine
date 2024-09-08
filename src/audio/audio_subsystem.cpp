// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "audio_subsystem.hpp"
#include "../scene/scene.hpp"
#include "../scripting/system_variable.hpp"

#include <mimalloc.h>

namespace lu::audio {
    static const system_variable<std::uint32_t> sv_max_audio_channels {"audio.max_channels", {512}};

    static auto print_audio_driver_info(FMOD::System* sys, int id) -> void;
    static auto print_speaker_mode_info(FMOD_SPEAKERMODE mode) -> void;
    static auto print_audio_interface_info(FMOD_OUTPUTTYPE type) -> void;

    audio_subsystem::audio_subsystem() : subsystem{"Audio"} {
        log_info("Initializing audio engine v.{:#x}", FMOD_VERSION);
        FMOD::Memory_Initialize(
            nullptr,
            0,
            +[](std::uint32_t size, FMOD_MEMORY_TYPE, const char*) noexcept -> void* { return ::mi_malloc(size); },
            +[](void* ptr, std::uint32_t size, FMOD_MEMORY_TYPE, const char*) noexcept -> void* { return ::mi_realloc(ptr, size); },
            +[](void* ptr, FMOD_MEMORY_TYPE, const char*) noexcept -> void { ::mi_free(ptr); }
        );
        fmod_check(FMOD::System_Create(&m_system, FMOD_VERSION));
        fmod_check(m_system->init(sv_max_audio_channels(), FMOD_INIT_NORMAL, nullptr));
        if (unsigned version = 0; m_system->getVersion(&version) == FMOD_RESULT::FMOD_OK) [[likely]] {
            log_info("Runtime Audio system version: {:#x}", version);
        }
        if (FMOD_OUTPUTTYPE type {}; m_system->getOutput(&type) == FMOD_RESULT::FMOD_OK) [[likely]] {
            print_audio_interface_info(type);
        }
        if (int driverID = 0; m_system->getDriver(&driverID) == FMOD_RESULT::FMOD_OK) [[likely]] {
            print_audio_driver_info(m_system, driverID);
        }

        m_test.emplace("/RES/audio/doggo.mp3");
        m_audio_source.clip = &*m_test;
    }

    audio_subsystem::~audio_subsystem() {
        log_info("Shutting down audio engine");
        m_test.reset();
        fmod_check(m_system->close());
        fmod_check(m_system->release());
    }

    auto audio_subsystem::on_post_tick() -> void {
        scene::get_active().each([&](const com::transform& transform, const com::audio_listener&) {
            set_audio_listener_transform(transform);
        });
        fmod_check(m_system->update());
    }

    auto audio_subsystem::set_audio_listener_transform(const com::transform& transform) noexcept -> void {
        static_assert(sizeof(FMOD_VECTOR) == sizeof(XMFLOAT3));
        static constinit XMVECTOR prev_position {};
        FMOD_VECTOR f_pos, f_vel, f_for, f_up;
        XMVECTOR position { XMLoadFloat4(&transform.position) };
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&f_pos), position);
        XMVECTOR velocity_delta = prev_position;
        velocity_delta = XMVectorSubtract(position, velocity_delta);
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&f_vel), velocity_delta);
        prev_position = position;
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&f_for), transform.forward_vec());
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&f_up), transform.up_vec());
        fmod_check(m_system->set3DListenerAttributes(
            0,
            &f_pos,
            &f_vel,
            &f_for,
            &f_up
        ));
    }

    auto audio_subsystem::on_start(scene& scene) -> void {
        m_audio_source.play();
    }

    static auto print_audio_driver_info(FMOD::System* const sys, const int id) -> void {
        if (!sys) [[unlikely]] return;
        eastl::array<char, 4096> name_buf {};
        FMOD_GUID guid {};
        int rate {};
        FMOD_SPEAKERMODE mode {};
        int channels {};
        const FMOD_RESULT result = sys->getDriverInfo(
            id,
            name_buf.data(),
            name_buf.size(),
            &guid,
            &rate,
            &mode,
            &channels
        );
        if(result != FMOD_RESULT::FMOD_OK) [[unlikely]] return;
        log_info("Audio device: {}", name_buf.data());
        static_assert(sizeof(FMOD_GUID) == 16);
        auto buf {eastl::bit_cast<eastl::array<std::uint64_t, 2>>(guid)};
        log_info("UUID: {:X}{:X}", buf[0], buf[1]);
        log_info("Rate: {} KHz", static_cast<double>(rate) / 1000.0);
        print_speaker_mode_info(mode);
        log_info("Speaker mode channels: {}", channels);
    }

    static auto print_speaker_mode_info(const FMOD_SPEAKERMODE mode) -> void {
        const char* msg = "Default";
        switch (mode) {
            default:
            case FMOD_SPEAKERMODE_DEFAULT: break;
            case FMOD_SPEAKERMODE_RAW:              msg = "Raw";        break;
            case FMOD_SPEAKERMODE_MONO:             msg = "Mono";       break;
            case FMOD_SPEAKERMODE_STEREO:           msg = "Stereo";     break;
            case FMOD_SPEAKERMODE_QUAD:             msg = "Quad";       break;
            case FMOD_SPEAKERMODE_SURROUND:         msg = "Surround";   break;
            case FMOD_SPEAKERMODE_5POINT1:          msg = "5.1";        break;
            case FMOD_SPEAKERMODE_7POINT1:          msg = "7.1";        break;
            case FMOD_SPEAKERMODE_7POINT1POINT4:    msg = "7.1.4";      break;
        }
        log_info("Speaker mode: {}", msg);
    }

    static auto print_audio_interface_info(const FMOD_OUTPUTTYPE type) -> void {
        const char* msg = "Unknown";
        switch (type) {
            case FMOD_OUTPUTTYPE_AUTODETECT:    msg = "AutoDetect";     break;
            case FMOD_OUTPUTTYPE_UNKNOWN:       msg = "Unknown";        break;
            case FMOD_OUTPUTTYPE_NOSOUND:       msg = "NOSOUND";        break;
            case FMOD_OUTPUTTYPE_WAVWRITER:     msg = "WAVWRITER";      break;
            case FMOD_OUTPUTTYPE_NOSOUND_NRT:   msg = "NOSOUND_NRT";    break;
            case FMOD_OUTPUTTYPE_WAVWRITER_NRT: msg = "WAVWRITER_NRT";  break;
            case FMOD_OUTPUTTYPE_WASAPI:        msg = "WASAPI";         break;
            case FMOD_OUTPUTTYPE_ASIO:          msg = "ASIO";           break;
            case FMOD_OUTPUTTYPE_PULSEAUDIO:    msg = "PULSEAUDIO";     break;
            case FMOD_OUTPUTTYPE_ALSA:          msg = "ALSA";           break;
            case FMOD_OUTPUTTYPE_COREAUDIO:     msg = "COREAUDIO";      break;
            case FMOD_OUTPUTTYPE_AUDIOTRACK:    msg = "AUDIOTRACK";     break;
            case FMOD_OUTPUTTYPE_OPENSL:        msg = "OPENSL";         break;
            case FMOD_OUTPUTTYPE_AUDIOOUT:      msg = "AUDIOOUT";       break;
            case FMOD_OUTPUTTYPE_AUDIO3D:       msg = "AUDIO3D";        break;
            case FMOD_OUTPUTTYPE_WEBAUDIO:      msg = "WEBAUDIO";       break;
            case FMOD_OUTPUTTYPE_NNAUDIO:       msg = "NNAUDIO";        break;
            case FMOD_OUTPUTTYPE_WINSONIC:      msg = "WINSONIC";       break;
            case FMOD_OUTPUTTYPE_AAUDIO:        msg = "AAUDIO";         break;
            default: break;
        }
        log_info("Output interface: {}", msg);
    }
}
