#include "ProjectHistory.h"
#include "Logger/LoggerService.h"
#include <algorithm>
#include <filesystem>
#include <utility>
#include <vector>
#include <fstream>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "ProjectHistory"

namespace Editor {

    void ProjectHistory::push(const std::filesystem::path &path) {
        // remove existing entry if exists
        std::erase_if(m_Entries, [&](const ProjectHistoryEntry &entry) { return entry.filePath == path; });

        auto entry = pathToEntry(path);
        if (!entry) {
            return;
        }
        m_Entries.push_back(std::move(*entry));
        prune();
        sortEntries(m_Entries);
    };

    bool ProjectHistory::Serialize(const std::filesystem::path &path) const {
        std::vector<ProjectHistoryEntry> sortedEntries = m_Entries;

        sortEntries(sortedEntries);

        // keep only the newest entries
        if (sortedEntries.size() > MAX_PROJECT_HISTFILE) {
            sortedEntries.resize(MAX_PROJECT_HISTFILE);
        }

        std::ofstream file(path);

        if (!file.is_open()) {
            return false;
        }

        for (const auto &entry : sortedEntries) {
            file << entry.filePath.string() << '\n';

            if (!file) {
                LOG_ERROR(LOG_WHO, "Failed to write project history.");
                return false;
            }
        }

        return true;
    }

    bool ProjectHistory::Deserialize(const std::filesystem::path &path) {
        std::vector<ProjectHistoryEntry> entries;

        std::ifstream file(path);

        if (!file.is_open()) {
            return false;
        }

        std::string line;

        while (std::getline(file, line)) {
            if (line.empty()) {
                continue;
            }

            const std::filesystem::path projectPath(line);

            auto entry = pathToEntry(projectPath);

            if (entry) {
                entries.push_back(std::move(*entry));
            }
        }

        // Newest -> oldest
        sortEntries(entries);

        // only keep the newest
        if (entries.size() > MAX_PROJECT_HISTFILE) {
            entries.resize(MAX_PROJECT_HISTFILE);
        }
        m_Entries = std::move(entries);
        return true;
    }

    std::optional<ProjectHistoryEntry> ProjectHistory::pathToEntry(const std::filesystem::path &path) {
        try {
            ProjectHistoryEntry entry;
            entry.filePath = path;
            entry.timestamp = std::filesystem::last_write_time(path);
            entry.ProjectConfig = Config::ProjectConfig::Deserialize(path.string());
            return entry;
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, e.what());
            return std::nullopt;
        }
    }

    void ProjectHistory::sortEntries(std::vector<ProjectHistoryEntry> &entries) {
        std::ranges::sort(entries, [](const ProjectHistoryEntry &a, const ProjectHistoryEntry &b) { return a.timestamp > b.timestamp; });
    }

    void ProjectHistory::prune() {
        if (m_Entries.size() > MAX_PROJECT_HISTFILE) {
            m_Entries.resize(MAX_PROJECT_HISTFILE);
        }
    }


} // namespace Editor

#pragma pop_macro("LOG_WHO")
