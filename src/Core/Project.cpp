#include "Project.h"
#include "IO/VFS.h"
#include <iostream>

std::shared_ptr<Project> Project::NewProject(const std::filesystem::path &baseDir, const std::string &name) {
    auto project = std::make_shared<Project>();

    std::filesystem::path projectDir = baseDir / name;
    project->m_ProjectFilepath = projectDir / "project.json";

    project->m_Config.windowTitle = name;
    project->m_Config.startScenePath = "assets/scenes/default.json";
    // todo init start scene w map etc

    std::filesystem::create_directories(projectDir);
    std::filesystem::create_directories(projectDir / "assets");
    std::filesystem::create_directories(projectDir / "assets/textures");
    std::filesystem::create_directories(projectDir / "assets/shaders");
    std::filesystem::create_directories(projectDir / "assets/scenes");
    std::filesystem::create_directories(projectDir / "assets/audio");

    IO::VFS::MountProject(project->m_ProjectFilepath.string());
    if (project->Save()) {
        return project;
    }
    return nullptr;
}

std::shared_ptr<Project> Project::Load(const std::filesystem::path &projectFilePath) {
    auto project = std::make_shared<Project>();
    project->m_ProjectFilepath = std::filesystem::absolute(projectFilePath);

    IO::VFS::MountProject(project->m_ProjectFilepath.string());

    project->m_Config = ProjectConfig::Deserialize("project.json");
    return project;
}

bool Project::Save() {
    return ProjectConfig::Serialize(m_Config, "project.json");
}
