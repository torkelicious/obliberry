#pragma once

#include "Core/ProjectConfig.h"
#include <filesystem>
#include <memory>
#include <string>

namespace Core {
    class Project {
    public:
        Project() = default;

        ~Project() = default;

        static std::shared_ptr<Project> NewProject(const std::filesystem::path &baseDir, const std::string &name);

        static std::shared_ptr<Project> Load(const std::filesystem::path &projectFilePath);

        [[nodiscard]] bool Save() const;

        static std::shared_ptr<Project> GetActive() { return s_ActiveProject; }
        static void SetActive(const std::shared_ptr<Project> &project) { s_ActiveProject = project; }

        [[nodiscard]] const ProjectConfig &GetConfig() const { return m_Config; }
        ProjectConfig &GetConfig() { return m_Config; }

        [[nodiscard]] std::filesystem::path GetProjectPath() const { return m_ProjectFilepath; }
        [[nodiscard]] std::filesystem::path GetRootDirectory() const { return m_ProjectFilepath.parent_path(); }
        [[nodiscard]] std::filesystem::path GetAssetsDirectory() const { return GetRootDirectory() / "assets"; }

    private:
        ProjectConfig m_Config;
        std::filesystem::path m_ProjectFilepath;
        static inline std::shared_ptr<Project> s_ActiveProject = nullptr;
    };
} // namespace Core
