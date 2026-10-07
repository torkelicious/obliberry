#include "TemporaryProject.h"
#include "Sound/AudioEngine.h"
#include <memory>

class AudioIntegrationTests : public TemporaryProject {};

TEST_F(AudioIntegrationTests, MissingAndMalformedAudioDoesNotBreakSubsequentCalls) {
    auto engine = Sound::AudioEngine::Create();
    if (!engine) {
        GTEST_SKIP() << "No audio backend could initialize.";
    }
    ASSERT_NO_FATAL_FAILURE(Write("invalid.wav", "not an audio file"));
    EXPECT_NO_THROW(engine->PlaySound2D("missing.wav"));
    EXPECT_NO_THROW(engine->PlayMusic("missing.wav"));
    EXPECT_NO_THROW(engine->PlaySound2D("invalid.wav"));
    EXPECT_NO_THROW(engine->PlayMusic("invalid.wav"));
    EXPECT_NO_THROW(engine->Update());
    EXPECT_NO_THROW(engine->StopMusic());
    EXPECT_NO_THROW(engine->StopMusic());
    EXPECT_NO_THROW(engine->SetMasterVolume(0.5f));
}
