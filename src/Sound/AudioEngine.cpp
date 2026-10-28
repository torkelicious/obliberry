#include "AudioEngine.h"

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <utility>

#include "IO/VFS/VFS.h"
#include "Logger/LoggerService.h"
#include "miniaudio.h"

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
    m_ActiveSounds.clear();

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
        const SoundInstance &sfx = **it;
        if (sfx.AtEnd() || !sfx.IsPlaying()) {
            it = m_ActiveSounds.erase(it);
        } else {
            ++it;
        }
    }
}

void Sound::AudioEngine::PlaySound2D(const std::string &filepath, const float volume, const bool persist) {
    if (!m_Engine)
        return;

    auto sfx = std::make_unique<SoundInstance>();
    sfx->persistent = persist;

    if (IO::VFS::IsPackaged()) {
        auto rawData = IO::VFS::ReadVirtual(filepath);
        if (!rawData.has_value()) {
            LOG_ERROR(LOG_WHO, "Sound not found in VFS: " + filepath);
            return;
        }

        sfx->decoder = new ma_decoder();
        const ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_unknown, 0, 0);

        ma_result result = ma_decoder_init_memory(rawData->data(), rawData->size(), &decoderConfig, sfx->decoder);
        if (result != MA_SUCCESS) {
            LOG_ERROR(LOG_WHO, "Failed to decode memory for: " + filepath);
            delete sfx->decoder;
            sfx->decoder = nullptr;
            return;
        }

        result = ma_sound_init_from_data_source(m_Engine, sfx->decoder, 0, nullptr, &sfx->sound);
        if (result != MA_SUCCESS) {
            LOG_ERROR(LOG_WHO, "Failed to initialize sound effect: " + filepath);
            return;
        }

        sfx->buffer = std::move(*rawData);
    } else {
        const std::filesystem::path absolutePath = IO::VFS::Resolve(filepath);

        const ma_result result = ma_sound_init_from_file(m_Engine, absolutePath.string().c_str(), 0, nullptr, nullptr, &sfx->sound);
        if (result != MA_SUCCESS) {
            LOG_ERROR(LOG_WHO, "Failed to initialize sound effect: " + filepath);
            return;
        }
    }

    sfx->SetVolume(volume);
    sfx->Start();

    std::lock_guard lock(m_Mutex);
    m_ActiveSounds.push_back(std::move(sfx));
}

void Sound::AudioEngine::PlayMusic(const std::string &filepath, const float volume) {
    if (!m_Engine)
        return;

    auto music = std::make_unique<SoundInstance>();

    if (IO::VFS::IsPackaged()) {
        auto rawData = IO::VFS::ReadVirtual(filepath);
        if (!rawData.has_value()) {
            LOG_ERROR(LOG_WHO, "Music not found in VFS: " + filepath);
            return;
        }

        music->decoder = new ma_decoder();
        const ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_unknown, 0, 0);

        ma_result result = ma_decoder_init_memory(rawData->data(), rawData->size(), &decoderConfig, music->decoder);
        if (result != MA_SUCCESS) {
            LOG_ERROR(LOG_WHO, "Failed to decode music memory: " + filepath);
            delete music->decoder;
            music->decoder = nullptr;
            return;
        }

        result = ma_sound_init_from_data_source(m_Engine, music->decoder, MA_SOUND_FLAG_STREAM, nullptr, &music->sound);
        if (result != MA_SUCCESS) {
            LOG_ERROR(LOG_WHO, "Failed to initialize music: " + filepath);
            return;
        }

        music->buffer = std::move(*rawData);
    } else {
        const std::filesystem::path absolutePath = IO::VFS::Resolve(filepath);

        const ma_result result = ma_sound_init_from_file(m_Engine, absolutePath.string().c_str(), MA_SOUND_FLAG_STREAM, nullptr, nullptr, &music->sound);
        if (result != MA_SUCCESS) {
            LOG_ERROR(LOG_WHO, "Failed to initialize music: " + filepath);
            return;
        }
    }

    music->SetLooping(true);
    music->SetVolume(volume);

    {
        std::lock_guard lock(m_Mutex);

        StopMusicUnlocked();

        m_CurrentMusic = std::move(music);
        m_CurrentMusic->Start();
    }
}

void Sound::AudioEngine::StopMusic() {
    std::lock_guard lock(m_Mutex);
    StopMusicUnlocked();
}

void Sound::AudioEngine::StopMusicUnlocked() { m_CurrentMusic.reset(); }

void Sound::AudioEngine::SetMasterVolume(const float volume) const {
    if (!m_Engine)
        return;
    ma_engine_set_volume(m_Engine, volume);
}

void Sound::AudioEngine::StopAll(bool respectPersist) {
    StopMusic();
    StopAllSfx(respectPersist);
}

void Sound::AudioEngine::StopAllSfx(bool respectPersist) {
    std::lock_guard lock(m_Mutex);

    if (!respectPersist) {
        m_ActiveSounds.clear();
        return;
    }

    std::erase_if(m_ActiveSounds, [](const std::unique_ptr<SoundInstance> &sfx) { return !sfx->persistent; });
}

void Sound::AudioEngine::StopSound(SoundInstance *sound) {
    if (!sound)
        return;

    std::lock_guard lock(m_Mutex);

    const auto it = std::find_if(m_ActiveSounds.begin(), m_ActiveSounds.end(), [sound](const std::unique_ptr<SoundInstance> &instance) { return instance.get() == sound; });

    if (it == m_ActiveSounds.end())
        return;

    m_ActiveSounds.erase(it);
}

#pragma pop_macro("LOG_WHO")
