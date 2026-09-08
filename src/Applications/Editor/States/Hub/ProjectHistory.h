
#pragma once

#include "Config/ProjectConfig.h"
#include "Core/Utils/PathUtils.h"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <vector>

namespace Editor {

    constexpr size_t MAX_PROJECT_HISTFILE = 5;

    struct ProjectHistoryEntry {
        std::filesystem::path filePath;            // project.json
        std::filesystem::file_time_type timestamp; // last modified
        Config::ProjectConfig ProjectConfig;       // project config of the file
    };

    class ProjectHistory {
    public:
        ProjectHistory() = default;
        void push(const std::filesystem::path &path);
        void pop() { m_Entries.pop_back(); }
        void remove_at(const size_t index) { m_Entries.erase(m_Entries.begin() + index); }

        bool Serialize(const std::filesystem::path &path = Core::PathUtils::GetExecutableDirectory() / "PROJECTHIST") const;

        bool Deserialize(const std::filesystem::path &path = Core::PathUtils::GetExecutableDirectory() / "PROJECTHIST");

        ProjectHistoryEntry &operator[](const size_t &idx) { return m_Entries[idx]; }
        const ProjectHistoryEntry &operator[](const size_t &idx) const { return m_Entries[idx]; }

        size_t size() const { return m_Entries.size(); }
        bool empty() const { return m_Entries.empty(); }

    private:
        static std::optional<ProjectHistoryEntry> pathToEntry(const std::filesystem::path &path);
        static void sortEntries(std::vector<ProjectHistoryEntry> &entries);
        void prune();
        std::vector<ProjectHistoryEntry> m_Entries;
    };


} // namespace Editor
