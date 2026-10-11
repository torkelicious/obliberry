#include "TemporaryProject.h"
#include "SaveData/SaveGameManager.h"
#include "SaveData/SaveGameSerialization.h"
#include <atomic>
#include <barrier>
#include <cstdint>
#include <thread>
#include <vector>

class SaveConcurrencyTests : public TemporaryProject {
protected:
    Saves::SaveGameManager manager;
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(TemporaryProject::SetUp());
        manager.Configure(directory / "saves");
    }
    void TearDown() override {
        manager.Reset();
        TemporaryProject::TearDown();
    }
};

TEST_F(SaveConcurrencyTests, ConcurrentSaveActiveCallsLeaveACompleteSaveAndNoTemporaryFiles) {
    manager.Set("score", std::int64_t{42});
    const auto filename = manager.CreateSave("Concurrent");
    ASSERT_TRUE(filename);
    std::atomic<int> failures{0};
    std::barrier start{4};
    std::vector<std::jthread> writers;
    for (int w = 0; w < 4; ++w) {
        writers.emplace_back([&] {
            start.arrive_and_wait();
            for (int i = 0; i < 20; ++i) {
                if (!manager.SaveActive()) {
                    ++failures;
                }
            }
        });
    }
    writers.clear();
    EXPECT_EQ(failures, 0);
    const auto saved = Saves::IO::Read(directory / "saves" / *filename);
    ASSERT_TRUE(saved);
    EXPECT_EQ(saved->values.at("score"), Saves::SaveValue{std::int64_t{42}});
    EXPECT_EQ(manager.GetActiveFilename(), filename);
    size_t files = 0;
    for (const auto &entry : std::filesystem::directory_iterator(directory / "saves")) {
        EXPECT_EQ(entry.path().extension(), ".json");
        ++files;
    }
    EXPECT_EQ(files, 1u);
}

TEST_F(SaveConcurrencyTests, ConcurrentCreateSaveCallsKeepEverySaveDistinctAndReadable) {
    manager.Set("score", std::int64_t{42});
    std::atomic<int> failures{0};
    std::barrier start{4};
    std::vector<std::jthread> writers;
    for (int w = 0; w < 4; ++w) {
        writers.emplace_back([&, w] {
            start.arrive_and_wait();
            for (int i = 0; i < 5; ++i) {
                if (!manager.CreateSave("Worker " + std::to_string(w))) {
                    ++failures;
                }
            }
        });
    }
    writers.clear();
    EXPECT_EQ(failures, 0);
    const auto saves = manager.ListSaves();
    ASSERT_EQ(saves.size(), 20u);
    for (const auto &save : saves) {
        const auto data = Saves::IO::Read(directory / "saves" / save.filename);
        ASSERT_TRUE(data);
        EXPECT_EQ(data->values.at("score"), Saves::SaveValue{std::int64_t{42}});
    }
}

TEST_F(SaveConcurrencyTests, ConcurrentValueChangesAndSavingPreserveFinalValues) {
    ASSERT_TRUE(manager.CreateSave("Values"));
    std::atomic<int> failures{0};
    std::barrier start{2};
    std::jthread values([&] {
        start.arrive_and_wait();
        for (int i = 0; i < 200; ++i) {
            manager.Set("counter", std::int64_t{i});
        }
    });
    std::jthread saves([&] {
        start.arrive_and_wait();
        for (int i = 0; i < 20; ++i) {
            if (!manager.SaveActive()) {
                ++failures;
            }
        }
    });
    values.join();
    saves.join();
    ASSERT_EQ(failures, 0);
    ASSERT_TRUE(manager.SaveActive());
    const auto filename = manager.GetActiveFilename();
    ASSERT_TRUE(filename);
    ASSERT_TRUE(manager.LoadSave(*filename));
    EXPECT_EQ(manager.Get("counter"), (std::optional<Saves::SaveValue>{std::int64_t{199}}));
}
