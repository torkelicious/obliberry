#include "AudioEngine.h"

#include <filesystem>

#include "miniaudio.h"
#include <iostream>

#include "IO/VFS.h"

std::unique_ptr<AudioEngine> AudioEngine::Create() {
    std::unique_ptr<AudioEngine> instance(new AudioEngine());

    if (!instance->Init()) {
        std::cerr << "[AudioEngine] Initialization failed.\n";
        return nullptr;
    }

    return instance;
}

AudioEngine::AudioEngine() = default;

bool AudioEngine::Init() {
    m_Engine = new ma_engine();

    if (const ma_result result = ma_engine_init(nullptr, m_Engine); result != MA_SUCCESS) {
        std::cerr << "[AudioEngine] miniaudio backend failed: Error code: " << result << "\n";
        delete m_Engine;
        m_Engine = nullptr;
        return false;
    }

    return true;
}

AudioEngine::~AudioEngine() {
    StopMusic();

    // forcing cleanup on any sound effects still playing
    for (ma_sound *sound: m_ActiveSounds) {
        ma_sound_uninit(sound);
        delete sound;
    }
    m_ActiveSounds.clear();

    // hardware backend context
    if (m_Engine) {
        ma_engine_uninit(m_Engine);
        delete m_Engine;
        m_Engine = nullptr;
    }
}

void AudioEngine::Update() {
    if (!m_Engine) return;

    // cleaning finished sound effects
    for (auto it = m_ActiveSounds.begin(); it != m_ActiveSounds.end();) {
        if (ma_sound_at_end(*it)) {
            ma_sound_uninit(*it);
            delete *it; // free allocated memory
            it = m_ActiveSounds.erase(it);
        } else {
            ++it;
        }
    }
}

void AudioEngine::PlaySound2D(const std::string &filepath, const float volume) {
    if (!m_Engine) return;

    const std::filesystem::path absolutePath = IO::VFS::Resolve(filepath);
    const std::string osPathString = absolutePath.string();

    const auto sfx = new ma_sound();
    const ma_result result = ma_sound_init_from_file(
        m_Engine,
        osPathString.c_str(),
        MA_SOUND_FLAG_DECODE,
        nullptr,
        nullptr,
        sfx
    );

    if (result != MA_SUCCESS) {
        std::cerr << "[AudioEngine] Failed to load sound effect: " << filepath
                << " (" << osPathString << ")\n";
        delete sfx;
        return;
    }

    ma_sound_set_volume(sfx, volume);
    ma_sound_start(sfx);

    m_ActiveSounds.push_back(sfx);
}

void AudioEngine::PlayMusic(const std::string &virtualPath, const float volume) {
    if (!m_Engine) return;

    // avoid overlapping music
    StopMusic();

    const std::filesystem::path absolutePath = IO::VFS::Resolve(virtualPath);
    const std::string osPathString = absolutePath.string();

    m_CurrentMusic = new ma_sound();

    const ma_result result = ma_sound_init_from_file(
        m_Engine,
        osPathString.c_str(),
        MA_SOUND_FLAG_STREAM,
        nullptr,
        nullptr,
        m_CurrentMusic
    );

    if (result != MA_SUCCESS) {
        std::cerr << "[AudioEngine] Failed to load music: " << virtualPath
                << " (" << osPathString << ")\n";
        delete m_CurrentMusic;
        m_CurrentMusic = nullptr;
        return;
    }

    ma_sound_set_looping(m_CurrentMusic, MA_TRUE);
    ma_sound_set_volume(m_CurrentMusic, volume);
    ma_sound_start(m_CurrentMusic);
}

void AudioEngine::StopMusic() {
    if (m_CurrentMusic) {
        ma_sound_stop(m_CurrentMusic);
        ma_sound_uninit(m_CurrentMusic);
        delete m_CurrentMusic;
        m_CurrentMusic = nullptr;
    }
}

void AudioEngine::SetMasterVolume(const float volume) const {
    if (!m_Engine) return;
    ma_engine_set_volume(m_Engine, volume);
}
