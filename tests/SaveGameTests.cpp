#include "Core/Utils/UUID.h"
#include "Core/Utils/PathUtils.h"
#include "Config/ProjectConfig.h"
#include "SaveData/SaveGameData.h"
#include "SaveData/SaveGameManager.h"
#include "SaveData/SaveGameSerialization.h"
#include <nlohmann/json.hpp>

#include <limits>
#include <optional>
#include <array>
#include <iterator>
#include <utility>
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

    void WriteListedSave(const std::filesystem::path &filename, const std::string &displayName, const std::int64_t created, const std::int64_t updated) {
        Saves::SaveData data;
        data.displayName = displayName;
        data.createdAtUtc = created;
        data.updatedAtUtc = updated;
        data.values = expectedValues;
        ASSERT_TRUE(Saves::IO::WriteAtomic(directory / filename, data));
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

TEST_F(SaveGameTests, SerializationRejectsInvalidVersions) {
    const std::vector<Json> invalidVersions{-1, 0, static_cast<unsigned>(Saves::SAVE_FORMAT_VERSION) + 1u, 256, static_cast<double>(Saves::SAVE_FORMAT_VERSION), "1", true, nullptr};

    for (const auto &version : invalidVersions) {
        SCOPED_TRACE(version.dump());

        auto document = MakeValidDocument();
        document["version"] = version;

        ASSERT_NO_FATAL_FAILURE(ExpectRejectedDocument(document));
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

TEST_F(SaveGameTests, ListSavesReturnsCorrectMetadata) {
    ASSERT_NO_FATAL_FAILURE(WriteListedSave("save-metadata.json", "Named Save", 123, 456));

    const auto saves = manager.ListSaves();
    ASSERT_EQ(saves.size(), 1u);

    EXPECT_EQ(saves[0].filename, std::filesystem::path{"save-metadata.json"});
    EXPECT_EQ(saves[0].displayName, "Named Save");
    EXPECT_EQ(saves[0].createdAtUtc, 123);
    EXPECT_EQ(saves[0].updatedAtUtc, 456);
}

TEST_F(SaveGameTests, ListSavesSkipsInvalidFilesAndDirectories) {
    ASSERT_NO_FATAL_FAILURE(WriteListedSave("save-valid.json", "Valid Save", 10, 20));
    ASSERT_NO_FATAL_FAILURE(WriteListedSave("ordinary.json", "Wrong Prefix", 10, 20));
    ASSERT_NO_FATAL_FAILURE(WriteListedSave("save-wrong.txt", "Wrong Extension", 10, 20));

    const std::array malformedFiles{std::pair{"save-broken.json", "{ broken json"}, std::pair{"save-incomplete.json", "{}"}};

    for (const auto &[filename, contents] : malformedFiles) {
        SCOPED_TRACE(filename);

        std::ofstream file(directory / filename);
        ASSERT_TRUE(file.is_open());

        file << contents;
        file.close();

        ASSERT_FALSE(file.fail());
    }

    ASSERT_TRUE(std::filesystem::create_directory(directory / "save-folder.json"));

    const auto saves = manager.ListSaves();
    ASSERT_EQ(saves.size(), 1u);

    EXPECT_EQ(saves[0].filename, std::filesystem::path{"save-valid.json"});
}

TEST_F(SaveGameTests, ListSavesSortsByUpdatedTimeNewestFirst) {
    ASSERT_NO_FATAL_FAILURE(WriteListedSave("save-a-middle.json", "Middle", 20, 200));
    ASSERT_NO_FATAL_FAILURE(WriteListedSave("save-m-newest.json", "Newest", 10, 300));
    ASSERT_NO_FATAL_FAILURE(WriteListedSave("save-z-oldest.json", "Oldest", 90, 100));

    const auto saves = manager.ListSaves();
    ASSERT_EQ(saves.size(), 3u);

    EXPECT_EQ(saves[0].filename, std::filesystem::path{"save-m-newest.json"});
    EXPECT_EQ(saves[1].filename, std::filesystem::path{"save-a-middle.json"});
    EXPECT_EQ(saves[2].filename, std::filesystem::path{"save-z-oldest.json"});

    EXPECT_EQ(saves[0].updatedAtUtc, 300);
    EXPECT_EQ(saves[1].updatedAtUtc, 200);
    EXPECT_EQ(saves[2].updatedAtUtc, 100);
}

TEST_F(SaveGameTests, CreatingSavesPreservesFilesAndTheirContents) {
    std::unordered_map<std::string, std::int64_t> created;

    for (std::int64_t i = 0; i < 32; ++i) {
        SCOPED_TRACE(i);
        manager.Set("SaveNumber", i);

        const auto filename = manager.CreateSave("Repeated Name");
        ASSERT_TRUE(filename.has_value());
        ASSERT_TRUE(created.emplace(filename->string(), i).second) << "Reused filename: " << filename->string();

        EXPECT_TRUE(std::filesystem::is_regular_file(directory / *filename));
    }

    ASSERT_EQ(manager.ListSaves().size(), created.size());
    manager.Set("SaveNumber", std::int64_t{-1});

    for (const auto &[filename, expected] : created) {
        SCOPED_TRACE(filename);

        ASSERT_TRUE(manager.LoadSave(filename));

        const auto actual = manager.Get("SaveNumber");
        ASSERT_TRUE(actual.has_value());
        EXPECT_EQ(*actual, Saves::SaveValue{expected});
    }
}

TEST_F(SaveGameTests, RemovingKeyPreservesOtherValues) {
    SetValues(expectedValues);

    ASSERT_TRUE(manager.Remove("IntegerValue"));

    EXPECT_FALSE(manager.Contains("IntegerValue"));
    EXPECT_FALSE(manager.Get("IntegerValue").has_value());

    auto remaining = expectedValues;
    remaining.erase("IntegerValue");

    ASSERT_NO_FATAL_FAILURE(ExpectValues(remaining));

    EXPECT_FALSE(manager.Remove("IntegerValue"));
    ASSERT_NO_FATAL_FAILURE(ExpectValues(remaining));
}


TEST_F(SaveGameTests, RemovingMissingKeyPreservesValues) {
    SetValues(expectedValues);

    EXPECT_FALSE(manager.Remove("MissingValue"));
    EXPECT_FALSE(manager.Remove(""));

    ASSERT_NO_FATAL_FAILURE(ExpectValues(expectedValues));
}

TEST_F(SaveGameTests, ClearValuesPreservesActiveFileAndConfiguration) {
    SetValues(expectedValues);

    const auto filename = manager.CreateSave("Keep Save");
    ASSERT_TRUE(filename.has_value());

    manager.ClearValues();

    for (const auto &[key, value] : expectedValues) {
        SCOPED_TRACE(key);

        EXPECT_FALSE(manager.Contains(key));
        EXPECT_FALSE(manager.Get(key).has_value());
    }

    EXPECT_TRUE(manager.IsConfigured());
    EXPECT_TRUE(manager.HasActiveFile());
    EXPECT_EQ(manager.GetActiveFilename(), filename);
    EXPECT_TRUE(std::filesystem::is_regular_file(directory / *filename));

    ASSERT_TRUE(manager.LoadSave(*filename));
    ASSERT_NO_FATAL_FAILURE(ExpectValues(expectedValues));
}

TEST_F(SaveGameTests, ResetClearsSessionAndConfigurationButPreservesFiles) {
    SetValues(expectedValues);

    const auto filename = manager.CreateSave("Keep Save");
    ASSERT_TRUE(filename.has_value());

    manager.Set("unsaved", true);
    manager.Reset();

    EXPECT_FALSE(manager.IsConfigured());
    EXPECT_FALSE(manager.HasActiveFile());
    EXPECT_FALSE(manager.GetActiveFilename().has_value());

    for (const auto &[key, value] : expectedValues) {
        SCOPED_TRACE(key);

        EXPECT_FALSE(manager.Contains(key));
        EXPECT_FALSE(manager.Get(key).has_value());
    }

    EXPECT_FALSE(manager.Contains("unsaved"));
    EXPECT_FALSE(manager.Get("unsaved").has_value());

    EXPECT_TRUE(manager.ListSaves().empty());
    EXPECT_FALSE(manager.SaveActive());
    EXPECT_FALSE(manager.LoadSave(*filename));
    EXPECT_FALSE(manager.DeleteSave(*filename));
    EXPECT_FALSE(manager.CreateSave("Unconfigured Save").has_value());

    ASSERT_TRUE(std::filesystem::is_regular_file(directory / *filename));

    ASSERT_NO_THROW(manager.Reset());

    manager.Configure(directory);

    ASSERT_TRUE(manager.LoadSave(*filename));
    ASSERT_NO_FATAL_FAILURE(ExpectValues(expectedValues));
    EXPECT_FALSE(manager.Contains("unsaved"));
}

TEST_F(SaveGameTests, FailedWritePreservesFileAndUnsavedValues) {
    SetValues(expectedValues);

    const auto filename = manager.CreateSave("Original Save");
    ASSERT_TRUE(filename.has_value());

    const auto path = directory / *filename;

    const auto readBytes = [&]() {
        std::ifstream file(path, std::ios::binary);
        return std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    };

    const auto originalBytes = readBytes();
    ASSERT_FALSE(originalBytes.empty());

    const auto changed = MakeChangedValues();
    SetValues(changed);

    auto temporaryPath = path;
    temporaryPath += ".tmp";
    ASSERT_TRUE(std::filesystem::create_directory(temporaryPath));

    ASSERT_FALSE(manager.SaveActive());

    EXPECT_EQ(readBytes(), originalBytes);
    EXPECT_EQ(manager.GetActiveFilename(), filename);
    EXPECT_TRUE(manager.HasActiveFile());
    ASSERT_NO_FATAL_FAILURE(ExpectValues(changed));

    ASSERT_TRUE(manager.LoadSave(*filename));
    ASSERT_NO_FATAL_FAILURE(ExpectValues(expectedValues));
    EXPECT_FALSE(manager.Contains("ExtraValue"));
}

namespace {
    constexpr const char *firstProjectUUID = "01234567-89ab-4cde-8fab-0123456789ab";
    constexpr const char *secondProjectUUID = "89abcdef-0123-4567-89ab-cdef01234567";

    constexpr std::array saveLocations{Config::SaveLocation::DataHome, Config::SaveLocation::Portable};
} // namespace

TEST(SaveDirectoryTests, DataHomeLocationUsesProjectUUID) {
    const auto expected = Core::PathUtils::GetDataHome() / "obliberry" / firstProjectUUID / "saves";

    const auto actual = Saves::IO::GenerateSaveDirectory(firstProjectUUID, Config::SaveLocation::DataHome);

    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(*actual, expected);
    EXPECT_TRUE(actual->is_absolute());
}

TEST(SaveDirectoryTests, PortableLocationUsesExecutableDirectory) {
    const auto expected = Core::PathUtils::GetExecutableDirectory() / "obliberry" / firstProjectUUID / "saves";

    const auto actual = Saves::IO::GenerateSaveDirectory(firstProjectUUID, Config::SaveLocation::Portable);

    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(*actual, expected);
    EXPECT_TRUE(actual->is_absolute());
}

TEST(SaveDirectoryTests, DifferentProjectUUIDsProduceSeparateDirectories) {
    for (const auto location : saveLocations) {
        SCOPED_TRACE(static_cast<unsigned>(location));

        const auto first = Saves::IO::GenerateSaveDirectory(firstProjectUUID, location);
        const auto second = Saves::IO::GenerateSaveDirectory(secondProjectUUID, location);
        const auto repeated = Saves::IO::GenerateSaveDirectory(firstProjectUUID, location);

        ASSERT_TRUE(first.has_value());
        ASSERT_TRUE(second.has_value());
        ASSERT_TRUE(repeated.has_value());

        EXPECT_NE(*first, *second);
        EXPECT_EQ(*first, *repeated);
        EXPECT_EQ(first->parent_path().filename(), std::filesystem::path{firstProjectUUID});
        EXPECT_EQ(second->parent_path().filename(), std::filesystem::path{secondProjectUUID});
    }
}

TEST(SaveDirectoryTests, InvalidProjectUUIDsAreRejected) {
    const std::vector<std::string> invalidUUIDs{"", "not-a-uuid", "../outside", "01234567-89ab-4cde-8fab-0123456789a", "01234567-89ab-4cde-8fab-0123456789ab0", "01234567_89ab-4cde-8fab-0123456789ab",
            "01234567-89ab-4cde-8fab-0123456789ag", "01234567-89ab-3cde-8fab-0123456789ab", "01234567-89ab-4cde-7fab-0123456789ab", "01234567-89ab-4cde-cfab-0123456789ab"};

    for (const auto location : saveLocations) {
        for (const auto &uuid : invalidUUIDs) {
            SCOPED_TRACE(testing::Message() << "Location: " << static_cast<unsigned>(location) << ", UUID: " << uuid);

            std::optional<std::filesystem::path> actual;
            ASSERT_NO_THROW(actual = Saves::IO::GenerateSaveDirectory(uuid, location));
            EXPECT_FALSE(actual.has_value());
        }
    }
}

TEST(SaveDirectoryTests, ValidUUIDVariantsAndUppercaseAreAccepted) {
    const std::array validUUIDs{"01234567-89ab-4cde-8fab-0123456789ab", "01234567-89ab-4cde-9fab-0123456789ab", "01234567-89ab-4cde-afab-0123456789ab", "01234567-89ab-4cde-bfab-0123456789ab",
            "01234567-89AB-4CDE-AFAB-0123456789AB", "01234567-89AB-4CDE-BFAB-0123456789AB"};

    for (const auto location : saveLocations) {
        for (const char *uuid : validUUIDs) {
            SCOPED_TRACE(testing::Message() << "Location: " << static_cast<unsigned>(location) << ", UUID: " << uuid);

            const auto actual = Saves::IO::GenerateSaveDirectory(uuid, location);
            ASSERT_TRUE(actual.has_value());
            EXPECT_EQ(actual->parent_path().filename(), std::filesystem::path{uuid});
            EXPECT_EQ(actual->filename(), std::filesystem::path{"saves"});
        }
    }
}

TEST(SaveDirectoryTests, InvalidSaveLocationsAreRejected) {
    for (const unsigned value : {2u, 255u}) {
        SCOPED_TRACE(value);
        const auto location = static_cast<Config::SaveLocation>(value);

        std::optional<std::filesystem::path> actual;
        ASSERT_NO_THROW(actual = Saves::IO::GenerateSaveDirectory(firstProjectUUID, location));
        EXPECT_FALSE(actual.has_value());
    }
}
