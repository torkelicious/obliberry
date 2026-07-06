#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

struct ma_engine;
struct ma_sound;
struct ma_decoder;

namespace Sound {
    class AudioEngine {
    public:
        static std::unique_ptr<AudioEngine> Create();

        ~AudioEngine();

        AudioEngine(const AudioEngine &) = delete;

        AudioEngine &operator=(const AudioEngine &) = delete;

        void Update();

        void PlaySound2D(const std::string &filepath, float volume = 1.0f);

        void PlayMusic(const std::string &filepath, float volume = 1.0f);

        void StopMusic();

        void SetMasterVolume(float volume) const;

    private:
        AudioEngine();

        bool Init();

        ma_engine *m_Engine = nullptr;
        ma_sound *m_CurrentMusic = nullptr;
        std::vector<ma_sound *> m_ActiveSounds;

        struct MemoryAudioContext {
            std::string buffer;
            ma_decoder *decoder = nullptr;
        };

        MemoryAudioContext m_CurrentMusicContext;
        std::unordered_map<ma_sound *, MemoryAudioContext> m_SoundContexts;
    };
} // namespace Sound
