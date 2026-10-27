#pragma once

#include "miniaudio.h"
#include <string>
#include <memory>
#include <mutex>
#include <vector>

struct ma_engine;

namespace Sound {

    struct SoundInstance {
        ma_sound sound{};
        ma_decoder *decoder = nullptr;
        std::string buffer;
        bool persistent = false;

        SoundInstance() = default;
        ~SoundInstance() {
            ma_sound_uninit(&sound);
            if (decoder) {
                ma_decoder_uninit(decoder);
                delete decoder;
            }
        }

        SoundInstance(const SoundInstance &) = delete;
        SoundInstance &operator=(const SoundInstance &) = delete;

        void Start() { ma_sound_start(&sound); }

        void Stop() { ma_sound_stop(&sound); }

        void SetVolume(float volume) { ma_sound_set_volume(&sound, volume); }

        void SetLooping(bool looping) { ma_sound_set_looping(&sound, looping ? MA_TRUE : MA_FALSE); }

        [[nodiscard]] bool IsPlaying() const { return ma_sound_is_playing(&sound) == MA_TRUE; }

        [[nodiscard]] bool AtEnd() const { return ma_sound_at_end(&sound) == MA_TRUE; }
    };

    class AudioEngine {
    public:
        static std::unique_ptr<AudioEngine> Create();

        ~AudioEngine();

        AudioEngine(const AudioEngine &) = delete;

        AudioEngine &operator=(const AudioEngine &) = delete;

        void Update();

        void PlaySound2D(const std::string &filepath, float volume = 1.0f, bool persist = false);

        void PlayMusic(const std::string &filepath, float volume = 1.0f);

        void StopMusic();

        void SetMasterVolume(float volume) const;

        void StopAll(bool respectPersist = false);

        void StopAllSfx(bool respectPersist = false);

        void StopSound(SoundInstance *sound);

    private:
        AudioEngine();

        bool Init();

        void StopMusicUnlocked();

        ma_engine *m_Engine = nullptr;

        mutable std::mutex m_Mutex;

        std::unique_ptr<SoundInstance> m_CurrentMusic;
        std::vector<std::unique_ptr<SoundInstance>> m_ActiveSounds;
    };
} // namespace Sound
