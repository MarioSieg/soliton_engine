// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "audio_clip.hpp"
#include "audio_subsystem.hpp"

namespace lu::audio {
    audio_clip::audio_clip(eastl::string&& asset_path, const audio_clip_descriptor& desc) : assetmgr::asset{assetmgr::asset_source::filesystem, eastl::move(asset_path)} {
        log_info("Loading audio clip '{}'", get_asset_path());
        eastl::vector<std::byte> buf {};
        assetmgr::with_primary_accessor_lock([&](assetmgr::asset_accessor &acc) {
            if (!acc.load_bin_file(get_asset_path().c_str(), buf)) {
                panic("Failed to load texture '{}'", get_asset_path());
            }
        });
        create_from_memory(buf, desc);
    }

    audio_clip::audio_clip(eastl::span<const std::byte> buf, const audio_clip_descriptor& desc) : assetmgr::asset{assetmgr::asset_source::memory} {
        create_from_memory(buf, desc);
    }

    audio_clip::~audio_clip() {
        if (m_sound) {
            m_sound->release();
        }
    }

    auto audio_clip::create_from_memory(const eastl::span<const std::byte> buf, const audio_clip_descriptor& desc) -> void {
        passert(!buf.empty() && !m_sound);
        FMOD_CREATESOUNDEXINFO info {};
        info.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
        info.length = buf.size();
        FMOD::System* const system = audio_subsystem::get_system();
        FMOD_MODE mode = FMOD_DEFAULT | FMOD_CREATESAMPLE | FMOD_OPENMEMORY;
        if (desc.enable_3d) mode |= FMOD_3D;
        if (desc.enable_looping) mode |= FMOD_LOOP_NORMAL;
        if (desc.enable_streaming) mode |= FMOD_CREATESTREAM;
        fmod_check(system->createSound(
            reinterpret_cast<const char*>(buf.data()),
            mode,
            &info,
            &m_sound
        ));
    }
}
