#pragma once

#include <string>
#include <memory>
#include <filesystem>
#include "Core/ProjectConfig.h"

class Project {
public:
    Project() = default;

    ~Project() = default;

    static std::shared_ptr<Project> NewProject(const std::filesystem::path &baseDir, const std::string &name);

    static std::shared_ptr<Project> Load(const std::filesystem::path &projectFilePath);

    bool Save();

    [[nodiscard]] const ProjectConfig &GetConfig() const { return m_Config; }
    ProjectConfig &GetConfig() { return m_Config; }

    [[nodiscard]] std::filesystem::path GetProjectPath() const { return m_ProjectFilepath; }
    [[nodiscard]] std::filesystem::path GetRootDirectory() const { return m_ProjectFilepath.parent_path(); }
    [[nodiscard]] std::filesystem::path GetAssetsDirectory() const { return GetRootDirectory() / "assets"; }

private:
    ProjectConfig m_Config;
    std::filesystem::path m_ProjectFilepath;
};
