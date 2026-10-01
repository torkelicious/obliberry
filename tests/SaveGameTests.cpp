#include "Core/Utils/UUID.h"
#include "SaveData/SaveGameData.h"
#include "SaveData/SaveGameManager.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <system_error>
#include <unordered_map>

class SaveGameTests : public testing::Test {
protected:
    using Values = std::unordered_map<std::string, Saves::SaveValue>;

    Saves::SaveGameManager manager;
    std::filesystem::path directory;
    bool ownsDirectory = false;

    const Values expectedValues = {{"IntegerValue", std::int64_t{99}}, {"StringValue", std::string{"thisIsMyString"}}, {"BooleanValue", false}, {"DoubleValue", 37.5}};

    void SetUp() override {
        directory = std::filesystem::temp_directory_path() / ("obliberry-save-test-" + Core::Utils::UUID::UUIDGenerator::Generate());

        ASSERT_TRUE(std::filesystem::create_directory(directory));
        ownsDirectory = true;
        manager.Configure(directory);
    }

    void TearDown() override {
        manager.Reset();

        if (ownsDirectory) {
            std::error_code err;
            std::filesystem::remove_all(directory, err);
            EXPECT_FALSE(err) << err.message();
        }
    }

    void SetValues(const Values &values) {
        for (const auto &[key, value] : values) {
            manager.Set(key, value);
        }
    }

    void ExpectValues(const Values &values) {
        for (const auto &[key, value] : values) {
            SCOPED_TRACE(key);

            const auto actual = manager.Get(key);
            ASSERT_TRUE(actual.has_value());
            EXPECT_EQ(*actual, value);
        }
    }
};

// saving and loading restores the original values and their types
TEST_F(SaveGameTests, ValuesSaveAndLoad) {
    SetValues(expectedValues);

    const auto filename = manager.CreateSave("Test Save");
    ASSERT_TRUE(filename.has_value());

    const Values changedValues = {{"IntegerValue", std::int64_t{67}}, {"StringValue", std::string{"wowThisIsDifferent"}}, {"BooleanValue", true}, {"DoubleValue", 12.25}, {"tempvalue", true}};

    SetValues(changedValues);

    ASSERT_TRUE(manager.LoadSave(*filename));
    ASSERT_NO_FATAL_FAILURE(ExpectValues(expectedValues));

    EXPECT_FALSE(manager.Contains("tempvalue"));
    EXPECT_EQ(manager.GetActiveFilename(), filename);
}

// A malformed load preserves the current values and active filename
TEST_F(SaveGameTests, FailedLoadPreservesCurrentState) {
    SetValues(expectedValues);

    const auto filename = manager.CreateSave("Existing Save");
    ASSERT_TRUE(filename.has_value());

    const Values currentValues = {{"IntegerValue", std::int64_t{19}}, {"StringValue", std::string{"Unsaved changes"}}, {"BooleanValue", true}, {"DoubleValue", 12.25}, {"unsaved", true}};

    SetValues(currentValues);

    const auto activeBefore = manager.GetActiveFilename();
    const std::filesystem::path malformedFilename = "save-malformed.json";

    {
        std::ofstream file(directory / malformedFilename);
        ASSERT_TRUE(file.is_open());

        file << "{ broken json";
        file.close();

        ASSERT_FALSE(file.fail());
    }

    EXPECT_FALSE(manager.LoadSave(malformedFilename));

    ASSERT_NO_FATAL_FAILURE(ExpectValues(currentValues));
    EXPECT_EQ(manager.GetActiveFilename(), activeBefore);
    EXPECT_TRUE(manager.HasActiveFile());
}

// Starting a new game clears memory without deleting existing saves
TEST_F(SaveGameTests, BeginNewGameClearsValuesAndActiveFilename) {
    SetValues(expectedValues);

    const auto filename = manager.CreateSave("Previous Game");
    ASSERT_TRUE(filename.has_value());

    manager.BeginNewGame();

    for (const auto &[key, value] : expectedValues) {
        EXPECT_FALSE(manager.Contains(key)) << key;
    }

    EXPECT_FALSE(manager.HasActiveFile());
    EXPECT_FALSE(manager.GetActiveFilename().has_value());
    EXPECT_TRUE(manager.IsConfigured());
    EXPECT_TRUE(std::filesystem::exists(directory / *filename));
}

// SaveActive requires an existing active filename
TEST_F(SaveGameTests, SaveActiveWithoutFilenameFails) {
    SetValues(expectedValues);

    EXPECT_FALSE(manager.SaveActive());
    EXPECT_FALSE(manager.HasActiveFile());
    EXPECT_FALSE(manager.GetActiveFilename().has_value());
    EXPECT_TRUE(std::filesystem::is_empty(directory));

    ASSERT_NO_FATAL_FAILURE(ExpectValues(expectedValues));
}

// Deleting the active file clears its filename but retains memory
TEST_F(SaveGameTests, DeletingActiveSaveClearsFilename) {
    SetValues(expectedValues);

    const auto filename = manager.CreateSave("Delete Me");
    ASSERT_TRUE(filename.has_value());
    ASSERT_TRUE(std::filesystem::exists(directory / *filename));

    ASSERT_TRUE(manager.DeleteSave(*filename));

    EXPECT_FALSE(std::filesystem::exists(directory / *filename));
    EXPECT_FALSE(manager.HasActiveFile());
    EXPECT_FALSE(manager.GetActiveFilename().has_value());

    ASSERT_NO_FATAL_FAILURE(ExpectValues(expectedValues));
}
