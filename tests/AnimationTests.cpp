#include <gtest/gtest.h>

#include "ECS/Systems/Animation/Animation.h"
#include "Rendering/Types/Texture/SpriteSheet.h"
#include "Rendering/Types/Texture/Texture.h"
#include "TestingUtils.h"

#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

class AnimationTests : public testing::Test {
protected:
    std::shared_ptr<Animation::SpriteAnimationSet> animations;
    Animation::Player player;

    std::shared_ptr<Rendering::SpriteSheet> MakeSheet(int width, int height, int columns, int rows, int columnSpacing = 0, int rowSpacing = 0) {
        auto sheet = std::make_shared<Rendering::SpriteSheet>();

        sheet->texture = std::make_shared<Rendering::Texture>(width, height, nullptr);

        sheet->columns = columns;
        sheet->rows = rows;
        sheet->columnSpacing = columnSpacing;
        sheet->rowSpacing = rowSpacing;

        return sheet;
    }

    void SetUp() override {
        animations = std::make_shared<Animation::SpriteAnimationSet>();
        animations->sheet = MakeSheet(32, 8, 4, 1);

        animations->clips["idle"] = Animation::SpriteClip{.frames = {{2, 0.125f}, {0, 0.25f}, {3, 0.125f}}, .loop = true};

        animations->clips["other"] = Animation::SpriteClip{.frames = {{1, 0.125f}}, .loop = true};

        player.animations = animations;

        ASSERT_TRUE(Animation::Play(player, "idle"));
    }

    void Advance(double dt) {
        const auto *clip = Animation::FindClip(player);
        ASSERT_NE(clip, nullptr);

        Animation::Advance(player, *clip, dt);
    }

    void ExpectPlayback(std::size_t frame, double elapsed, bool playing = true) {
        EXPECT_EQ(player.frameIndex, frame);
        EXPECT_DOUBLE_EQ(player.elapsed, elapsed);
        EXPECT_EQ(player.playing, playing);
    }
};

TEST_F(AnimationTests, FrameDurationsControlAdvancement) {
    Advance(0.0625);
    ExpectPlayback(0, 0.0625);

    Advance(0.0625);
    ExpectPlayback(1, 0.0);

    Advance(0.125);
    ExpectPlayback(1, 0.125);

    Advance(0.125);
    ExpectPlayback(2, 0.0);
}

TEST_F(AnimationTests, LargeTimeStepAdvancesAcrossMultipleCycles) {
    // Each cycle lasts 0.5 seconds.
    Advance(1000.4375);

    ExpectPlayback(2, 0.0625);
}

TEST_F(AnimationTests, LoopWrapsToFirstFrame) {
    Advance(0.4375);
    ExpectPlayback(2, 0.0625);

    Advance(0.0625);
    ExpectPlayback(0, 0.0);
}

TEST_F(AnimationTests, NonloopingClipStopsOnLastFrame) {
    animations->clips.at("idle").loop = false;

    Advance(0.5);
    ExpectPlayback(2, 0.0, false);

    Advance(1.0);
    ExpectPlayback(2, 0.0, false);
}

TEST_F(AnimationTests, PauseAndResumePreserveProgress) {
    Advance(0.1875);

    Animation::Pause(player);
    Advance(1.0);

    ExpectPlayback(1, 0.0625, false);

    Animation::Resume(player);
    ExpectPlayback(1, 0.0625);

    Advance(0.1875);
    ExpectPlayback(2, 0.0);
}

TEST_F(AnimationTests, StopResetsAndPreventsAdvancement) {
    Advance(0.1875);

    Animation::Stop(player);
    ExpectPlayback(0, 0.0, false);

    Advance(1.0);
    ExpectPlayback(0, 0.0, false);
}

TEST_F(AnimationTests, PlayingSameClipKeepsProgress) {
    Advance(0.1875);

    ASSERT_TRUE(Animation::Play(player, "idle"));

    ExpectPlayback(1, 0.0625);
}

TEST_F(AnimationTests, ExplicitRestartResetsSameClip) {
    Advance(0.1875);
    Animation::Pause(player);

    ASSERT_TRUE(Animation::Play(player, "idle", Animation::RestartSetting::Restart));

    ExpectPlayback(0, 0.0);
}

TEST_F(AnimationTests, SwitchingClipResetsAndStartsPlayback) {
    Advance(0.1875);
    Animation::Pause(player);

    ASSERT_TRUE(Animation::Play(player, "other"));

    EXPECT_EQ(player.clip, "other");
    ExpectPlayback(0, 0.0);
}

TEST_F(AnimationTests, MissingClipPreservesCurrentPlayback) {
    Advance(0.1875);

    EXPECT_FALSE(Animation::Play(player, "missing"));

    EXPECT_EQ(player.clip, "idle");
    ExpectPlayback(1, 0.0625);
}

TEST_F(AnimationTests, InvalidTimeStepsPreserveProgress) {
    Advance(0.0625);

    const std::vector<double> invalidTimes{0.0, -1.0, std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()};

    for (const double dt : invalidTimes) {
        SCOPED_TRACE(dt);

        Advance(dt);
        ExpectPlayback(0, 0.0625);
    }
}

TEST_F(AnimationTests, ResolvePoseUsesSpriteIndexFromCurrentFrame) {
    Advance(0.125);

    Animation::Sprite sprite;
    ASSERT_TRUE(Animation::ResolvePose(player, sprite));

    EXPECT_EQ(player.frameIndex, 1u);
    EXPECT_EQ(sprite.frame, 0u);
    EXPECT_EQ(sprite.sheet, animations->sheet);
}

TEST_F(AnimationTests, ResetToInitialRespectsAutoplay) {
    for (const bool autoplay : {false, true}) {
        SCOPED_TRACE(autoplay);

        player.initialClip = "idle";
        player.autoplay = autoplay;
        player.clip = "idle";
        player.frameIndex = 2;
        player.elapsed = 0.0625;
        player.playing = true;

        ASSERT_TRUE(Animation::ResetToInitial(player));

        EXPECT_EQ(player.clip, "idle");
        ExpectPlayback(0, 0.0, autoplay);
    }
}

TEST_F(AnimationTests, InvalidClipsAreRejectedWithoutChangingPlayback) {
    animations->clips["bad"] = Animation::SpriteClip{};

    EXPECT_FALSE(Animation::Play(player, "bad"));
    EXPECT_EQ(player.clip, "idle");
    ExpectPlayback(0, 0.0);

    const std::vector<Animation::SpriteFrame> invalidFrames{{4, 0.125f}, {0, 0.0f}, {0, -0.125f}, {0, std::numeric_limits<float>::infinity()}, {0, std::numeric_limits<float>::quiet_NaN()}};

    for (std::size_t i = 0; i < invalidFrames.size(); ++i) {
        SCOPED_TRACE(i);

        animations->clips["bad"] = Animation::SpriteClip{.frames = {invalidFrames[i]}, .loop = true};

        EXPECT_FALSE(Animation::Play(player, "bad"));
        EXPECT_EQ(player.clip, "idle");
        ExpectPlayback(0, 0.0);
    }
}

TEST_F(AnimationTests, SheetUVsAccountForSpacing) {
    const auto sheet = MakeSheet(34, 18, 4, 2, 2, 2);

    ASSERT_TRUE(Animation::ValidateSheet(*sheet));

    const glm::vec4 expected{9.0f / 34.0f, 0.0f, 7.0f / 34.0f, 8.0f / 18.0f};

    TestUtils::GLM_VecExpectFloat(sheet->GetFrameUV(5), expected);
}
