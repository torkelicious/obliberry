#include "Project.h"
#include "IO/VFS.h"
#include <filesystem>
#include <iostream>

std::shared_ptr<Project> Project::NewProject(const std::filesystem::path &baseDir, const std::string &name) {
    auto project = std::make_shared<Project>();
    const std::filesystem::path projectDir = baseDir / name;
    project->m_ProjectFilepath = projectDir / "project.json";
    project->m_Config.windowTitle = name;
    project->m_Config.startScenePath = "assets/scenes/default.json";

    const std::filesystem::path templateDir = "Templates/Default";
    if (!std::filesystem::exists(templateDir)) {
        std::cerr << "[Project] Error: Template directory not found at: " << std::filesystem::absolute(templateDir)
                << "\n";
        return nullptr;
    }

    try {
        std::filesystem::create_directories(projectDir);
        for (const auto &entry: std::filesystem::directory_iterator(templateDir)) {
            const auto &srcPath = entry.path();
            const auto destPath = projectDir / srcPath.filename();
            std::filesystem::remove_all(destPath);
            std::filesystem::copy(srcPath, destPath,
                                  std::filesystem::copy_options::recursive |
                                  std::filesystem::copy_options::overwrite_existing);
        }
    } catch (const std::exception &e) {
        std::cerr << "[Project] Failed to copy template project: " << e.what() << "\n";
        return nullptr;
    }

    IO::VFS::MountProject(project->m_ProjectFilepath.string());
    if (project->Save()) {
        s_ActiveProject = project;
        return project;
    }
    return nullptr;
}

std::shared_ptr<Project> Project::Load(const std::filesystem::path &projectFilePath) {
    auto project = std::make_shared<Project>();
    project->m_ProjectFilepath = std::filesystem::absolute(projectFilePath);

    IO::VFS::MountProject(project->m_ProjectFilepath.string());
    project->m_Config = ProjectConfig::Deserialize("project.json");
    s_ActiveProject = project;
    return project;
}

bool Project::Save() const { return ProjectConfig::Serialize(m_Config, "project.json"); }
