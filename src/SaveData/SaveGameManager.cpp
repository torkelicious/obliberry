#include "SaveGameManager.h"

#include "Logger/LoggerService.h"
#include "SaveData/SaveGameSerialization.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "SaveGameManager"

namespace Saves {

    namespace {
        std::int64_t CurrentUtcMilliseconds() {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        }
    } // namespace

    SaveGameManager::SaveGameManager(std::filesystem::path saveDirectory) { Configure(std::move(saveDirectory)); }

    void SaveGameManager::Configure(std::filesystem::path saveDirectory) {
        if (saveDirectory.empty()) {
            throw std::invalid_argument("Save directory cannot be empty");
        }

        std::error_code error;
        std::filesystem::create_directories(saveDirectory, error);
        if (error) {
            throw std::filesystem::filesystem_error("Could not create save directory", saveDirectory, error);
        }

        std::unique_lock lock(m_DataMutex);
        m_SaveDirectory = std::move(saveDirectory);
        m_ActiveSave = SaveData{};
        m_ActiveFilename.reset();
    }

    void SaveGameManager::Reset() {
        std::unique_lock lock(m_DataMutex);
        m_ActiveSave = SaveData{};
        m_ActiveFilename.reset();
        m_SaveDirectory.clear();
    }

    bool SaveGameManager::IsConfigured() const {
        std::shared_lock lock(m_DataMutex);
        return !m_SaveDirectory.empty();
    }

    void SaveGameManager::BeginNewGame(std::string startingScene) {
        std::unique_lock lock(m_DataMutex);
        m_ActiveSave = SaveData{};
        m_ActiveSave.currentScene = std::move(startingScene);
        m_ActiveFilename.reset();
    }

    bool SaveGameManager::HasActiveFile() const {
        std::shared_lock lock(m_DataMutex);
        return m_ActiveFilename.has_value();
    }

    void SaveGameManager::Set(std::string key, SaveValue value) {
        if (key.empty()) {
            return;
        }

        std::unique_lock lock(m_DataMutex);
        m_ActiveSave.values.insert_or_assign(std::move(key), std::move(value));
    }

    std::optional<SaveValue> SaveGameManager::Get(std::string_view key) const {
        std::shared_lock lock(m_DataMutex);

        const auto found = m_ActiveSave.values.find(std::string(key));
        if (found == m_ActiveSave.values.end()) {
            return std::nullopt;
        }
        return found->second;
    }

    bool SaveGameManager::Contains(std::string_view key) const {
        std::shared_lock lock(m_DataMutex);

        return m_ActiveSave.values.contains(std::string(key));
    }

    bool SaveGameManager::Remove(std::string_view key) {
        std::unique_lock lock(m_DataMutex);
        return m_ActiveSave.values.erase(std::string(key)) != 0;
    }

    void SaveGameManager::ClearValues() {
        std::unique_lock lock(m_DataMutex);
        m_ActiveSave.values.clear();
    }

    void SaveGameManager::SetCurrentScene(std::string scene) {
        std::unique_lock lock(m_DataMutex);
        m_ActiveSave.currentScene = std::move(scene);
    }

    std::string SaveGameManager::GetCurrentScene() const {
        std::shared_lock lock(m_DataMutex);
        return m_ActiveSave.currentScene;
    }

    std::optional<std::filesystem::path> SaveGameManager::CreateSave(std::string displayName) {
        const std::filesystem::path filename = GenerateFilename();
        if (filename.empty()) {
            LOG_ERROR(LOG_WHO, "Could not generate a save filename");
            return std::nullopt;
        }

        const auto resolved = ResolveFilename(filename);
        if (!resolved) {
            LOG_ERROR(LOG_WHO, "Could not resolve save filename");
            return std::nullopt;
        }

        SaveData snapshot = Snapshot();

        const auto now = CurrentUtcMilliseconds();
        snapshot.displayName = std::move(displayName);
        snapshot.createdAtUtc = now;
        snapshot.updatedAtUtc = now;

        if (!IO::WriteAtomic(*resolved, snapshot)) {
            return std::nullopt;
        }

        {
            std::unique_lock lock(m_DataMutex);
            m_ActiveSave.displayName = snapshot.displayName;
            m_ActiveSave.createdAtUtc = snapshot.createdAtUtc;
            m_ActiveSave.updatedAtUtc = snapshot.updatedAtUtc;
            m_ActiveFilename = filename;
        }
        return filename;
    }

    bool SaveGameManager::SaveActive() {
        std::optional<std::filesystem::path> filename;
        SaveData snapshot;
        {
            std::shared_lock lock(m_DataMutex);
            filename = m_ActiveFilename;
            snapshot = m_ActiveSave;
        }
        if (!filename) {
            LOG_ERROR(LOG_WHO, "Cannot save: no active file.");
            return false;
        }

        const auto resolved = ResolveFilename(*filename);
        if (!resolved) {
            return false;
        }
        snapshot.updatedAtUtc = CurrentUtcMilliseconds();
        if (!IO::WriteAtomic(*resolved, snapshot)) {
            return false;
        }

        {
            std::unique_lock lock(m_DataMutex);
            if (m_ActiveFilename == filename) {
                m_ActiveSave.updatedAtUtc = snapshot.updatedAtUtc;
            }
        }
        return true;
    }

    bool SaveGameManager::LoadSave(const std::filesystem::path &filename) {
        const auto resolved = ResolveFilename(filename);

        if (!resolved) {
            LOG_ERROR(LOG_WHO, "Invalid file: " + filename.string());
            return false;
        }

        auto loaded = IO::Read(*resolved);
        if (!loaded) {
            return false;
        }

        {
            std::unique_lock lock(m_DataMutex);
            m_ActiveSave = std::move(*loaded);
            m_ActiveFilename = filename.filename();
        }
        return true;
    }

    bool SaveGameManager::DeleteSave(const std::filesystem::path &filename) {
        const auto resolved = ResolveFilename(filename);

        if (!resolved) {
            return false;
        }

        std::error_code error;
        const bool removed = std::filesystem::remove(*resolved, error);
        if (error || !removed) {
            LOG_ERROR(LOG_WHO, "Could not delete save file: " + resolved->string());
            return false;
        }

        {
            std::unique_lock lock(m_DataMutex);
            if (m_ActiveFilename == filename.filename()) {
                m_ActiveFilename.reset();
            }
        }

        return true;
    }

    std::vector<SaveInfo> SaveGameManager::ListSaves() const {
        std::filesystem::path dir;
        {
            std::shared_lock lock(m_DataMutex);
            dir = m_SaveDirectory;
        }

        std::vector<SaveInfo> saves;

        if (dir.empty()) {
            return saves;
        }

        std::error_code iterr;

        std::filesystem::directory_iterator iterator(dir, iterr);
        const std::filesystem::directory_iterator end;

        while (!iterr && iterator != end) {
            const auto &entry = *iterator;
            std::error_code entryerr;
            if (entry.is_regular_file(entryerr) && !entryerr) {
                const auto name = entry.path().filename();
                const std::string nameStr = name.string();
                if (name.extension() == ".json" && nameStr.starts_with("save-")) {
                    const auto data = IO::Read(entry.path());
                    if (data) {
                        saves.push_back(SaveInfo{
                                .filename = name,
                                .displayName = data->displayName,
                                .currentScene = data->currentScene,
                                .createdAtUtc = data->createdAtUtc,
                                .updatedAtUtc = data->updatedAtUtc,
                        });
                    }
                }
            }
            iterator.increment(iterr);
        }
        if (iterr) {
            LOG_ERROR(LOG_WHO, "Could not iterate save files: " + iterr.message());
        }

        std::ranges::sort(saves, [](const SaveInfo &lhs, const SaveInfo &rhs) { return lhs.updatedAtUtc > rhs.updatedAtUtc; });
        return saves;
    }

    std::optional<std::filesystem::path> SaveGameManager::GetActiveFilename() const {
        std::shared_lock lock(m_DataMutex);
        return m_ActiveFilename;
    }

    std::filesystem::path SaveGameManager::GenerateFilename() const {
        std::filesystem::path savedir;
        {
            std::shared_lock lock(m_DataMutex);
            savedir = m_SaveDirectory;
        }
        if (savedir.empty()) {
            return {};
        }
        const auto time = CurrentUtcMilliseconds();
        std::filesystem::path name = "save-" + std::to_string(time) + ".json";

        std::error_code error;
        const bool initialExists = std::filesystem::exists(savedir / name, error);
        if (error) {
            LOG_ERROR(LOG_WHO, "Could not check save filename: " + error.message());
            return {};
        }

        if (!initialExists) {
            return name;
        }

        for (std::uint32_t suffix = 1; suffix != 0; ++suffix) {
            name = "save-" + std::to_string(time) + "-" + std::to_string(suffix) + ".json";

            error.clear();
            const bool exists = std::filesystem::exists(savedir / name, error);
            if (error) {
                LOG_ERROR(LOG_WHO, "Could not check save filename: " + error.message());
                return {};
            }

            if (!exists) {
                return name;
            }
        }

        LOG_ERROR(LOG_WHO, "Could not generate a unique save filename");
        return {};
    }

    std::optional<std::filesystem::path> SaveGameManager::ResolveFilename(const std::filesystem::path &filename) const {

        if (filename.empty() || filename.is_absolute() || filename.has_root_path() || filename.has_parent_path() || filename != filename.filename()) {
            return std::nullopt;
        }

        if (filename.extension() != ".json") {
            return std::nullopt;
        }

        const std::string text = filename.string();

        if (!text.starts_with("save-")) {
            return std::nullopt;
        }

        std::shared_lock lock(m_DataMutex);

        if (m_SaveDirectory.empty()) {
            return std::nullopt;
        }

        return m_SaveDirectory / filename;
    }

    SaveData SaveGameManager::Snapshot() const {
        std::shared_lock lock(m_DataMutex);
        return m_ActiveSave;
    }

} // namespace Saves

#pragma pop_macro("LOG_WHO")
