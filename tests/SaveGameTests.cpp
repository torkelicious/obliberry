#include "Core/Utils/UUID.h"
#include "SaveData/SaveGameData.h"
#include "SaveData/SaveGameManager.h"
#include "SaveData/SaveGameSerialization.h"
#include <nlohmann/json.hpp>

#include <limits>
#include <optional>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <system_error>
#include <unordered_map>

class SaveGameTests : public testing::Test {
protected:
    using Json = nlohmann::json;
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

    Values MakeChangedValues() const {
        auto values = expectedValues;
        values["IntegerValue"] = std::int64_t{123};
        values["ExtraValue"] = std::string("This is only in the changed one");
        return values;
    }

    Json MakeValidDocument() const { return {{"version", Saves::SAVE_FORMAT_VERSION}, {"name", "Validation Save"}, {"created_at", 100}, {"updated_at", 200}, {"values", {{"Value", 42}}}}; }

    void ExpectRejectedDocument(const Json &document) {
        const auto filename = directory / "save-validation.json";

        std::ofstream file(filename, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(file.is_open());

        file << document.dump();
        file.close();

        ASSERT_FALSE(file.fail());

        std::optional<Saves::SaveData> loaded;
        ASSERT_NO_THROW(loaded = Saves::IO::Read(filename));
        EXPECT_FALSE(loaded.has_value());
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

TEST_F(SaveGameTests, SaveActivePersistsChangedValues) {
    SetValues(expectedValues);
    const auto filename = manager.CreateSave("Test Save");
    ASSERT_TRUE(filename.has_value());

    const auto changed = MakeChangedValues();
    SetValues(changed);

    ASSERT_TRUE(manager.SaveActive());

    EXPECT_EQ(manager.GetActiveFilename(), filename);
    EXPECT_EQ(manager.ListSaves().size(), 1u);

    manager.BeginNewGame();

    ASSERT_TRUE(manager.LoadSave(*filename));
    ASSERT_NO_FATAL_FAILURE(ExpectValues(changed));
}

TEST_F(SaveGameTests, SeperateFilesRestoreOwnData) {
    SetValues(expectedValues);

    const auto first = manager.CreateSave("First Save");
    ASSERT_TRUE(first.has_value());

    const auto changedValues = MakeChangedValues();
    SetValues(changedValues);

    const auto second = manager.CreateSave("Second Save");
    ASSERT_TRUE(second.has_value());
    ASSERT_NE(*first, *second);

    ASSERT_TRUE(manager.LoadSave(*first));

    ASSERT_NO_FATAL_FAILURE(ExpectValues(expectedValues));
    EXPECT_FALSE(manager.Contains("ExtraValue"));
    EXPECT_EQ(manager.GetActiveFilename(), first);

    ASSERT_TRUE(manager.LoadSave(*second));

    ASSERT_NO_FATAL_FAILURE(ExpectValues(changedValues));
    EXPECT_EQ(manager.GetActiveFilename(), second);
}

TEST_F(SaveGameTests, MissingFileLoadPreservesCurrentState) {
    SetValues(expectedValues);

    const auto filename = manager.CreateSave("Existing Save");
    ASSERT_TRUE(filename.has_value());

    const auto currentValues = MakeChangedValues();
    SetValues(currentValues);

    EXPECT_FALSE(manager.LoadSave("save-missing.json"));

    ASSERT_NO_FATAL_FAILURE(ExpectValues(currentValues));
    EXPECT_EQ(manager.GetActiveFilename(), filename);
    EXPECT_TRUE(manager.HasActiveFile());
}

TEST_F(SaveGameTests, InvalidFilenamesPreserveStateAndFiles) {
    SetValues(expectedValues);

    const auto filename = manager.CreateSave("Existing Save");
    ASSERT_TRUE(filename.has_value());

    const auto currentValues = MakeChangedValues();
    SetValues(currentValues);

    const std::vector<std::filesystem::path> invalidFilenames{std::filesystem::path{},
            directory / *filename, // absolute path to a real save
            "../save-outside.json", "nested/save-test.json", "save-test.txt", "bad.json"};

    for (const auto &invalid : invalidFilenames) {
        SCOPED_TRACE(invalid.string());

        EXPECT_FALSE(manager.LoadSave(invalid));
        EXPECT_FALSE(manager.DeleteSave(invalid));

        ASSERT_NO_FATAL_FAILURE(ExpectValues(currentValues));
        EXPECT_EQ(manager.GetActiveFilename(), filename);
        EXPECT_TRUE(manager.HasActiveFile());

        ASSERT_TRUE(std::filesystem::exists(directory / *filename));
    }
}

TEST_F(SaveGameTests, DeletingInactiveSavePreservesActiveSave) {
    SetValues(expectedValues);

    const auto inactive = manager.CreateSave("Inactive Save");
    ASSERT_TRUE(inactive.has_value());

    const auto currentValues = MakeChangedValues();
    SetValues(currentValues);

    const auto active = manager.CreateSave("Active Save");
    ASSERT_TRUE(active.has_value());
    ASSERT_NE(*inactive, *active);

    ASSERT_TRUE(manager.DeleteSave(*inactive));

    EXPECT_FALSE(std::filesystem::exists(directory / *inactive));
    EXPECT_TRUE(std::filesystem::exists(directory / *active));
    EXPECT_EQ(manager.GetActiveFilename(), active);
    EXPECT_TRUE(manager.HasActiveFile());

    ASSERT_NO_FATAL_FAILURE(ExpectValues(currentValues));
}

TEST_F(SaveGameTests, DeletingMissingSavePreservesCurrentState) {
    SetValues(expectedValues);

    const auto filename = manager.CreateSave("Existing Save");
    ASSERT_TRUE(filename.has_value());

    EXPECT_FALSE(manager.DeleteSave("save-missing.json"));

    EXPECT_TRUE(std::filesystem::exists(directory / *filename));
    EXPECT_EQ(manager.GetActiveFilename(), filename);
    EXPECT_TRUE(manager.HasActiveFile());

    ASSERT_NO_FATAL_FAILURE(ExpectValues(expectedValues));
}

TEST_F(SaveGameTests, SerializationPreservesDataAndLimits) {
    Saves::SaveData expected;
    expected.displayName = "Boundary Test";
    expected.createdAtUtc = 100;
    expected.updatedAtUtc = 200;
    expected.values = expectedValues;

    expected.values["MinimumInteger"] = std::numeric_limits<std::int64_t>::min();

    expected.values["MaximumInteger"] = std::numeric_limits<std::int64_t>::max();

    expected.values["MaximumDouble"] = std::numeric_limits<double>::max();

    // fucked up text
    expected.values["EscapedText"] = std::string{"\xC3\xA5\xC3\xA4\xC3\xB6\n\"quoted\"\\path"};

    const auto filename = directory / "save-boundaries.json";

    ASSERT_TRUE(Saves::IO::WriteAtomic(filename, expected));

    const auto loaded = Saves::IO::Read(filename);
    ASSERT_TRUE(loaded.has_value());

    EXPECT_EQ(loaded->version, expected.version);
    EXPECT_EQ(loaded->displayName, expected.displayName);
    EXPECT_EQ(loaded->createdAtUtc, expected.createdAtUtc);
    EXPECT_EQ(loaded->updatedAtUtc, expected.updatedAtUtc);
    EXPECT_EQ(loaded->values, expected.values);
}

TEST_F(SaveGameTests, SerializationRejectsMissingFields) {
    for (const char *field : {"version", "name", "created_at", "updated_at", "values"}) {
        SCOPED_TRACE(field);
        auto doc = MakeValidDocument();
        doc.erase(field);
        ASSERT_NO_FATAL_FAILURE(ExpectRejectedDocument(doc));
    }
}

TEST_F(SaveGameTests, SerializationRejectsNonobjectDocuments) {
    const std::vector<Json> invalidDocuments{nullptr, true, 42, "text", Json::array()};

    for (const auto &document : invalidDocuments) {
        SCOPED_TRACE(document.dump());

        ASSERT_NO_FATAL_FAILURE(ExpectRejectedDocument(document));
    }
}

TEST_F(SaveGameTests, SerializationRejectsWrongFieldTypes) {
    struct Case {
        const char *field;
        Json value;
    };

    const std::vector<Case> cases{{"name", 42}, {"name", true}, {"name", nullptr}, {"values", Json::array()}, {"values", "text"}, {"values", false}, {"values", nullptr}};

    for (const auto &test : cases) {
        SCOPED_TRACE(testing::Message() << test.field << ": " << test.value.dump());

        auto document = MakeValidDocument();
        document[test.field] = test.value;

        ASSERT_NO_FATAL_FAILURE(ExpectRejectedDocument(document));
    }
}

TEST_F(SaveGameTests, SerializationRejectsInvalidTimestamps) {
    const std::vector<Json> invalidValues{-1, 1.5, "100", true, nullptr, std::numeric_limits<std::uint64_t>::max()};

    for (const char *field : {"created_at", "updated_at"}) {
        for (const auto &value : invalidValues) {
            SCOPED_TRACE(testing::Message() << field << ": " << value.dump());

            auto document = MakeValidDocument();
            document[field] = value;

            ASSERT_NO_FATAL_FAILURE(ExpectRejectedDocument(document));
        }
    }
}

TEST_F(SaveGameTests, SerializationRejectsUnsupportedValuesAndEmptyKeys) {
    const std::vector<Json> invalidValues{nullptr, Json::array({1, 2}), Json::object({{"nested", 1}}), std::numeric_limits<std::uint64_t>::max()};

    for (const auto &value : invalidValues) {
        SCOPED_TRACE(value.dump());

        auto document = MakeValidDocument();
        document["values"]["InvalidValue"] = value;

        ASSERT_NO_FATAL_FAILURE(ExpectRejectedDocument(document));
    }

    auto document = MakeValidDocument();
    document["values"][""] = 42;

    ASSERT_NO_FATAL_FAILURE(ExpectRejectedDocument(document));
}

TEST_F(SaveGameTests, NonfiniteWritePreservesExistingFile) {
    Saves::SaveData original;
    original.displayName = "Original";
    original.values = expectedValues;

    const auto filename = directory / "save-existing.json";
    ASSERT_TRUE(Saves::IO::WriteAtomic(filename, original));

    const std::vector<double> invalidValues{std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};

    for (const double value : invalidValues) {
        SCOPED_TRACE(value);

        auto invalid = original;
        invalid.displayName = "Should Not Replace Original";
        invalid.values["InvalidValue"] = value;

        EXPECT_FALSE(Saves::IO::WriteAtomic(filename, invalid));

        const auto loaded = Saves::IO::Read(filename);
        ASSERT_TRUE(loaded.has_value());

        EXPECT_EQ(loaded->displayName, original.displayName);
        EXPECT_EQ(loaded->values, original.values);
    }
}

TEST_F(SaveGameTests, EmptyKeyWritePreservesExistingFile) {
    Saves::SaveData original;
    original.displayName = "Original";
    original.values = expectedValues;

    const auto filename = directory / "save-existing.json";
    ASSERT_TRUE(Saves::IO::WriteAtomic(filename, original));

    auto invalid = original;
    invalid.displayName = "Should Not Replace Original";
    invalid.values[""] = true;

    EXPECT_FALSE(Saves::IO::WriteAtomic(filename, invalid));

    const auto loaded = Saves::IO::Read(filename);
    ASSERT_TRUE(loaded.has_value());

    EXPECT_EQ(loaded->displayName, original.displayName);
    EXPECT_EQ(loaded->values, original.values);
}
