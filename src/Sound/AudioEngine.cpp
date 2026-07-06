#include "AudioEngine.h"
#include <filesystem>
#include <iostream>
#include "IO/VFS.h"
#include "miniaudio.h"

std::unique_ptr<Sound::AudioEngine> Sound::AudioEngine::Create() {
    std::unique_ptr<AudioEngine> instance(new AudioEngine());
    if (!instance->Init()) {
        std::cerr << "[AudioEngine] Initialization failed.\n";
        return nullptr;
    }
    return instance;
}

Sound::AudioEngine::AudioEngine() = default;

bool Sound::AudioEngine::Init() {
    m_Engine = new ma_engine();
    if (const ma_result result = ma_engine_init(nullptr, m_Engine); result != MA_SUCCESS) {
        std::cerr << "[AudioEngine] miniaudio backend failed: Error code: " << result << "\n";
        delete m_Engine;
        m_Engine = nullptr;
        return false;
    }
    return true;
}

Sound::AudioEngine::~AudioEngine() {
    StopMusic();

    for (ma_sound *sound: m_ActiveSounds) {
        ma_sound_uninit(sound);
        if (m_SoundContexts.contains(sound)) {
            ma_decoder_uninit(m_SoundContexts[sound].decoder);
            delete m_SoundContexts[sound].decoder;
        }
        delete sound;
    }
    m_ActiveSounds.clear();
    m_SoundContexts.clear();

    if (m_Engine) {
        ma_engine_uninit(m_Engine);
        delete m_Engine;
        m_Engine = nullptr;
    }
}

void Sound::AudioEngine::Update() {
    if (!m_Engine) return;

    for (auto it = m_ActiveSounds.begin(); it != m_ActiveSounds.end();) {
        ma_sound *sound = *it;
        if (ma_sound_at_end(sound) || !ma_sound_is_playing(sound)) {
            ma_sound_uninit(sound);

            if (m_SoundContexts.contains(sound)) {
                ma_decoder_uninit(m_SoundContexts[sound].decoder);
                delete m_SoundContexts[sound].decoder;
                m_SoundContexts.erase(sound);
            }

            delete sound;
            it = m_ActiveSounds.erase(it);
        } else {
            ++it;
        }
    }
}

void Sound::AudioEngine::PlaySound2D(const std::string &filepath, const float volume) {
    if (!m_Engine) return;

    auto *sfx = new ma_sound();
    ma_result result;

    if (IO::VFS::IsPackaged()) {
        auto rawData = IO::VFS::ReadVirtual(filepath);
        if (!rawData.has_value()) {
            std::cerr << "[AudioEngine] Sound not found in VFS: " << filepath << "\n";
            delete sfx;
            return;
        }

        m_SoundContexts[sfx].buffer = std::move(rawData.value());
        const std::string &bufferRef = m_SoundContexts[sfx].buffer;

        auto *decoder = new ma_decoder();
        const ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_unknown, 0, 0);

        result = ma_decoder_init_memory(bufferRef.data(), bufferRef.size(), &decoderConfig, decoder);
        if (result != MA_SUCCESS) {
            std::cerr << "[AudioEngine] Failed to decode memory for: " << filepath << "\n";
            m_SoundContexts.erase(sfx);
            delete decoder;
            delete sfx;
            return;
        }

        m_SoundContexts[sfx].decoder = decoder;

        result = ma_sound_init_from_data_source(m_Engine, decoder, 0, nullptr, sfx);
    } else {
        const std::filesystem::path absolutePath = IO::VFS::Resolve(filepath);
        result = ma_sound_init_from_file(m_Engine, absolutePath.string().c_str(), 0, nullptr, nullptr, sfx);
    }

    if (result != MA_SUCCESS) {
        std::cerr << "[AudioEngine] Failed to initialize sound effect: " << filepath << "\n";
        if (m_SoundContexts.contains(sfx)) {
            ma_decoder_uninit(m_SoundContexts[sfx].decoder);
            delete m_SoundContexts[sfx].decoder;
            m_SoundContexts.erase(sfx);
        }
        delete sfx;
        return;
    }

    ma_sound_set_volume(sfx, volume);
    ma_sound_start(sfx);
    m_ActiveSounds.push_back(sfx);
}

void Sound::AudioEngine::PlayMusic(const std::string &filepath, const float volume) {
    if (!m_Engine) return;

    StopMusic();
    m_CurrentMusic = new ma_sound();
    ma_result result;

    if (IO::VFS::IsPackaged()) {
        auto rawData = IO::VFS::ReadVirtual(filepath);
        if (!rawData.has_value()) {
            std::cerr << "[AudioEngine] Music not found in VFS: " << filepath << "\n";
            delete m_CurrentMusic;
            m_CurrentMusic = nullptr;
            return;
        }

        m_CurrentMusicContext.buffer = std::move(rawData.value());

        auto *decoder = new ma_decoder();
        const ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_unknown, 0, 0);

        result = ma_decoder_init_memory(m_CurrentMusicContext.buffer.data(), m_CurrentMusicContext.buffer.size(),
                                        &decoderConfig, decoder);
        if (result != MA_SUCCESS) {
            std::cerr << "[AudioEngine] Failed to decode music memory: " << filepath << "\n";
            delete decoder;
            delete m_CurrentMusic;
            m_CurrentMusic = nullptr;
            m_CurrentMusicContext.buffer.clear();
            return;
        }

        m_CurrentMusicContext.decoder = decoder;

        result = ma_sound_init_from_data_source(m_Engine, decoder, MA_SOUND_FLAG_STREAM, nullptr, m_CurrentMusic);
    } else {
        const std::filesystem::path absolutePath = IO::VFS::Resolve(filepath);
        result = ma_sound_init_from_file(m_Engine, absolutePath.string().c_str(), MA_SOUND_FLAG_STREAM, nullptr,
                                         nullptr, m_CurrentMusic);
    }

    if (result != MA_SUCCESS) {
        std::cerr << "[AudioEngine] Failed to initialize music: " << filepath << "\n";
        if (m_CurrentMusicContext.decoder) {
            ma_decoder_uninit(m_CurrentMusicContext.decoder);
            delete m_CurrentMusicContext.decoder;
            m_CurrentMusicContext.decoder = nullptr;
        }
        delete m_CurrentMusic;
        m_CurrentMusic = nullptr;
        m_CurrentMusicContext.buffer.clear();
        return;
    }

    ma_sound_set_looping(m_CurrentMusic, MA_TRUE);
    ma_sound_set_volume(m_CurrentMusic, volume);
    ma_sound_start(m_CurrentMusic);
}

void Sound::AudioEngine::StopMusic() {
    if (m_CurrentMusic) {
        ma_sound_stop(m_CurrentMusic);
        ma_sound_uninit(m_CurrentMusic);
        delete m_CurrentMusic;
        m_CurrentMusic = nullptr;

        if (m_CurrentMusicContext.decoder) {
            ma_decoder_uninit(m_CurrentMusicContext.decoder);
            delete m_CurrentMusicContext.decoder;
            m_CurrentMusicContext.decoder = nullptr;
        }
    }
    m_CurrentMusicContext.buffer.clear();
}

void Sound::AudioEngine::SetMasterVolume(const float volume) const {
    if (!m_Engine) return;
    ma_engine_set_volume(m_Engine, volume);
}
