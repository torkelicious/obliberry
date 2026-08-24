#include "AudioEngine.h"
#include <filesystem>
#include "miniaudio.h"
#include "Logger/LoggerService.h"
#include "IO/VFS/VFS.h"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "AudioEngine"

// Larger period = fewer IPC roundtrips to the audio daemon per second
constexpr ma_uint32 AUDIO_PERIOD_FRAMES = 2048;

std::unique_ptr<Sound::AudioEngine> Sound::AudioEngine::Create() {
    std::unique_ptr<AudioEngine> instance(new AudioEngine());
    if (!instance->Init()) {
        LOG_ERROR(LOG_WHO, "Initialization failed");
        return nullptr;
    }
    return instance;
}

Sound::AudioEngine::AudioEngine() = default;

bool Sound::AudioEngine::Init() {
    m_Engine = new ma_engine();

    ma_engine_config config = ma_engine_config_init();
    config.periodSizeInFrames = AUDIO_PERIOD_FRAMES;

    if (const ma_result result = ma_engine_init(&config, m_Engine); result != MA_SUCCESS) {
        LOG_ERROR(LOG_WHO, "miniaudio backend failed: Error code: " + std::to_string(result));
        delete m_Engine;
        m_Engine = nullptr;
        return false;
    }
    return true;
}

Sound::AudioEngine::~AudioEngine() {
    StopMusic();

    std::lock_guard lock(m_Mutex);
    for (ma_sound *sound : m_ActiveSounds) {
        ma_sound_uninit(sound);
        if (const auto it = m_SoundContexts.find(sound); it != m_SoundContexts.end()) {
            ma_decoder_uninit(it->second.decoder);
            delete it->second.decoder;
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
    if (!m_Engine)
        return;

    std::lock_guard lock(m_Mutex);
    for (auto it = m_ActiveSounds.begin(); it != m_ActiveSounds.end();) {
        if (ma_sound *sound = *it; ma_sound_at_end(sound) || !ma_sound_is_playing(sound)) {
            ma_sound_uninit(sound);

            if (const auto ctxIt = m_SoundContexts.find(sound); ctxIt != m_SoundContexts.end()) {
                ma_decoder_uninit(ctxIt->second.decoder);
                delete ctxIt->second.decoder;
                m_SoundContexts.erase(ctxIt);
            }

            delete sound;
            it = m_ActiveSounds.erase(it);
        } else {
            ++it;
        }
    }
}

void Sound::AudioEngine::PlaySound2D(const std::string &filepath, const float volume) {
    if (!m_Engine)
        return;

    auto *sfx = new ma_sound();
    ma_result result;

    std::string localBuffer;
    ma_decoder *decoder = nullptr;

    if (IO::VFS::IsPackaged()) {
        auto rawData = IO::VFS::ReadVirtual(filepath);
        if (!rawData.has_value()) {
            LOG_ERROR(LOG_WHO, "Sound not found in VFS: " + filepath);
            delete sfx;
            return;
        }

        localBuffer = std::move(rawData.value());

        decoder = new ma_decoder();
        const ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_unknown, 0, 0);

        result = ma_decoder_init_memory(localBuffer.data(), localBuffer.size(), &decoderConfig, decoder);
        if (result != MA_SUCCESS) {
            LOG_ERROR(LOG_WHO, "Failed to decode memory for: " + filepath);
            delete decoder;
            delete sfx;
            return;
        }

        result = ma_sound_init_from_data_source(m_Engine, decoder, 0, nullptr, sfx);
    } else {
        const std::filesystem::path absolutePath = IO::VFS::Resolve(filepath);
        result = ma_sound_init_from_file(m_Engine, absolutePath.string().c_str(), 0, nullptr, nullptr, sfx);
    }

    if (result != MA_SUCCESS) {
        LOG_ERROR(LOG_WHO, "Failed to initialize sound effect: " + filepath);
        if (decoder) {
            ma_decoder_uninit(decoder);
            delete decoder;
        }
        delete sfx;
        return;
    }

    ma_sound_set_volume(sfx, volume);
    ma_sound_start(sfx);

    {
        std::lock_guard lock(m_Mutex);
        if (decoder) {
            m_SoundContexts[sfx] = MemoryAudioContext{std::move(localBuffer), decoder};
        }
        m_ActiveSounds.push_back(sfx);
    }
}

void Sound::AudioEngine::PlayMusic(const std::string &filepath, const float volume) {
    if (!m_Engine)
        return;

    StopMusic();

    auto *music = new ma_sound();
    ma_result result;
    std::string localBuffer;
    ma_decoder *decoder = nullptr;

    if (IO::VFS::IsPackaged()) {
        auto rawData = IO::VFS::ReadVirtual(filepath);
        if (!rawData.has_value()) {
            LOG_ERROR(LOG_WHO, "Music not found in VFS: " + filepath);
            delete music;
            return;
        }

        localBuffer = std::move(rawData.value());

        decoder = new ma_decoder();
        const ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_unknown, 0, 0);

        result = ma_decoder_init_memory(localBuffer.data(), localBuffer.size(), &decoderConfig, decoder);
        if (result != MA_SUCCESS) {
            LOG_ERROR(LOG_WHO, "Failed to decode music memory: " + filepath);
            delete decoder;
            delete music;
            return;
        }

        result = ma_sound_init_from_data_source(m_Engine, decoder, MA_SOUND_FLAG_STREAM, nullptr, music);
    } else {
        const std::filesystem::path absolutePath = IO::VFS::Resolve(filepath);
        result = ma_sound_init_from_file(m_Engine, absolutePath.string().c_str(), MA_SOUND_FLAG_STREAM, nullptr, nullptr, music);
    }

    if (result != MA_SUCCESS) {
        LOG_ERROR(LOG_WHO, "Failed to initialize music: " + filepath);
        if (decoder) {
            ma_decoder_uninit(decoder);
            delete decoder;
        }
        delete music;
        return;
    }

    ma_sound_set_looping(music, MA_TRUE);
    ma_sound_set_volume(music, volume);
    ma_sound_start(music);

    {
        std::lock_guard lock(m_Mutex);
        m_CurrentMusic = music;
        m_CurrentMusicContext = MemoryAudioContext{std::move(localBuffer), decoder};
    }
}

void Sound::AudioEngine::StopMusic() {
    std::lock_guard lock(m_Mutex);

    if (m_CurrentMusic) {
        ma_sound_stop(m_CurrentMusic);
        ma_sound_uninit(m_CurrentMusic);
        delete m_CurrentMusic;
    }
    if (m_CurrentMusicContext.decoder) {
        ma_decoder_uninit(m_CurrentMusicContext.decoder);
        delete m_CurrentMusicContext.decoder;
    }
    m_CurrentMusic = nullptr;
    m_CurrentMusicContext.decoder = nullptr;
    m_CurrentMusicContext.buffer.clear();
}

void Sound::AudioEngine::SetMasterVolume(const float volume) const {
    if (!m_Engine)
        return;
    ma_engine_set_volume(m_Engine, volume);
}
#pragma pop_macro("LOG_WHO")
