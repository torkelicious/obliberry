#pragma once

#include "SaveGameData.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>

namespace Saves {

    struct SaveInfo {
        std::filesystem::path filename;
        std::string displayName;
        std::string currentScene;
        std::int64_t createdAtUtc = 0;
        std::int64_t updatedAtUtc = 0;
    };

    class SaveGameManager {
    public:
        SaveGameManager() = default;

        explicit SaveGameManager(std::filesystem::path saveDirectory);

        // when opens a project
        void Configure(std::filesystem::path saveDirectory);
        void Reset();
        [[nodiscard]] bool IsConfigured() const;

        // active session
        void BeginNewGame(std::string startingScene);
        [[nodiscard]] bool HasActiveFile() const;

        // values
        void Set(std::string key, SaveValue value);
        [[nodiscard]] std::optional<SaveValue> Get(std::string_view key) const;
        [[nodiscard]] bool Contains(std::string_view key) const;
        bool Remove(std::string_view key);
        void ClearValues();

        // scene metadata
        void SetCurrentScene(std::string scene);
        [[nodiscard]] std::string GetCurrentScene() const;

        // files
        [[nodiscard]] std::optional<std::filesystem::path> CreateSave(std::string displayName);
        bool SaveActive();
        bool LoadSave(const std::filesystem::path &filename);
        bool DeleteSave(const std::filesystem::path &filename);

        [[nodiscard]] std::vector<SaveInfo> ListSaves() const;

        [[nodiscard]] std::optional<std::filesystem::path> GetActiveFilename() const;

    private:
        [[nodiscard]] std::filesystem::path GenerateFilename() const;

        [[nodiscard]] std::optional<std::filesystem::path> ResolveFilename(const std::filesystem::path &filename) const;

        [[nodiscard]] SaveData Snapshot() const;

        SaveData m_ActiveSave;
        std::optional<std::filesystem::path> m_ActiveFilename;
        std::filesystem::path m_SaveDirectory;
        mutable std::shared_mutex m_DataMutex;
    };

} // namespace Saves
