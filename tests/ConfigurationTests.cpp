#include "TemporaryProject.h"
#include "Config/ProjectConfig.h"
#include "Config/GraphicsConfig.h"
#include <limits>

class ConfigurationTests : public TemporaryProject {};

TEST_F(ConfigurationTests, ProjectConfigurationRoundtrip) {
    Config::ProjectConfig expected;
    expected.UUID = Core::Utils::UUID::UUIDGenerator::Generate();
    expected.Title = "Test Project";
    expected.startScenePath = "assets/scenes/start.json";
    expected.useSaves = true;
    expected.saveLocation = Config::SaveLocation::Portable;
    ASSERT_TRUE(Config::ProjectConfig::Serialize(expected));
    const auto actual = Config::ProjectConfig::Deserialize();
    EXPECT_EQ(actual.UUID, expected.UUID);
    EXPECT_EQ(actual.Title, expected.Title);
    EXPECT_EQ(actual.startScenePath, expected.startScenePath);
    EXPECT_EQ(actual.useSaves, expected.useSaves);
    EXPECT_EQ(actual.saveLocation, expected.saveLocation);
}

TEST_F(ConfigurationTests, ProjectDefaultsAndInvalidSaveLocations) {
    const Config::ProjectConfig defaults;
    ASSERT_NO_FATAL_FAILURE(WriteJson("project.json", nlohmann::json::object()));
    const auto actual = Config::ProjectConfig::Deserialize();
    EXPECT_EQ(actual.Title, defaults.Title);
    EXPECT_EQ(actual.startScenePath, defaults.startScenePath);
    EXPECT_EQ(actual.useSaves, defaults.useSaves);
    EXPECT_EQ(actual.saveLocation, defaults.saveLocation);
    for (const auto &invalid : std::vector<nlohmann::json>{-1, 2, 256, 1.5, "1", true, nullptr, std::numeric_limits<std::uint64_t>::max()}) {
        SCOPED_TRACE(invalid.dump());
        ASSERT_NO_FATAL_FAILURE(WriteJson("project.json", {{"saves", {{"enabled", true}, {"location", invalid}}}}));
        const auto loaded = Config::ProjectConfig::Deserialize();
        EXPECT_EQ(loaded.saveLocation, defaults.saveLocation);
        EXPECT_TRUE(loaded.useSaves);
    }
}

TEST_F(ConfigurationTests, GraphicsRoundtripAndDefaults) {
    const auto previousSamples = Config::GraphicsCapabilities::s_SupportedSampleCounts;
    struct Restore {
        std::vector<std::uint8_t> previous;
        ~Restore() { Config::GraphicsCapabilities::s_SupportedSampleCounts = previous; }
    } restore{previousSamples};
    Config::GraphicsCapabilities::s_SupportedSampleCounts = {1, 2, 4, 8};
    Config::GraphicsConfig expected;
    expected.WindowWidth = 1024;
    expected.WindowHeight = 768;
    expected.Fullscreen = true;
    expected.MSAAEnabled = true;
    expected.AASamples = 8;
    expected.TargetFPS = 144;
    expected.VSync = Config::VSyncType::ADAPTIVE;
    expected.ShowPerformanceOverlay = true;
    Config::GraphicsConfig::Serialize(expected);
    const auto actual = Config::GraphicsConfig::Deserialize();
    EXPECT_EQ(actual.WindowWidth, expected.WindowWidth);
    EXPECT_EQ(actual.WindowHeight, expected.WindowHeight);
    EXPECT_EQ(actual.Fullscreen, expected.Fullscreen);
    EXPECT_EQ(actual.MSAAEnabled, expected.MSAAEnabled);
    EXPECT_EQ(actual.AASamples, expected.AASamples);
    EXPECT_EQ(actual.TargetFPS, expected.TargetFPS);
    EXPECT_EQ(actual.VSync, expected.VSync);
    EXPECT_EQ(actual.ShowPerformanceOverlay, expected.ShowPerformanceOverlay);

    ASSERT_NO_FATAL_FAILURE(WriteJson("graphics.json", nlohmann::json::object()));
    const auto defaults = Config::GraphicsConfig::Deserialize();
    const Config::GraphicsConfig expectedDefaults;
    EXPECT_EQ(defaults.WindowWidth, expectedDefaults.WindowWidth);
    EXPECT_EQ(defaults.WindowHeight, expectedDefaults.WindowHeight);
    EXPECT_EQ(defaults.VSync, expectedDefaults.VSync);
    EXPECT_EQ(defaults.AASamples, expectedDefaults.AASamples);
}

TEST_F(ConfigurationTests, UnknownVsyncUsesDefaultAndMalformedFileFailsSafely) {
    ASSERT_NO_FATAL_FAILURE(WriteJson("graphics.json", {{"vsync", "invalid"}}));
    EXPECT_EQ(Config::GraphicsConfig::Deserialize().VSync, Config::VSyncType::STANDARD);
    ASSERT_NO_FATAL_FAILURE(Write("graphics.json", "{ broken"));
    EXPECT_NO_THROW(Config::GraphicsConfig::Deserialize());
    ASSERT_NO_FATAL_FAILURE(Write("project.json", "{ broken"));
    EXPECT_FALSE(Config::ProjectConfig::DeserializeFile(directory / "project.json"));
}
