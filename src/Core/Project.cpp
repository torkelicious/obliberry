#include "Project.h"
#include "Config/ProjectConfig.h"
#include "Core/Utils/PathUtils.h"
#include "Logger/LoggerService.h"
#include "IO/VFS/VFS.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <utility>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "Project"

namespace {
    // Templates
    std::string ReadTemplateTitle(const std::filesystem::path &templateDir) {
        const std::filesystem::path projectFile = templateDir / "project.json";
        if (!std::filesystem::exists(projectFile))
            return {};

        try {
            std::ifstream file(projectFile);
            nlohmann::json j;
            file >> j;
            if (j.contains("window") && j["window"].contains("title"))
                return j["window"]["title"].get<std::string>();
        } catch (const std::exception &e) {
            LOG_WARN(LOG_WHO, "Failed to parse template project.json: " + std::string(e.what()));
        }
        return {};
    }
} // namespace

namespace Core {

    std::vector<Project::TemplateInfo> Project::GetAvailableTemplates() {
        std::vector<TemplateInfo> templates;
        const std::filesystem::path templatesRoot = PathUtils::GetExecutableDirectory() / "Templates";
        if (!std::filesystem::exists(templatesRoot)) {
            LOG_WARN(LOG_WHO, "Templates directory not found at: " + std::filesystem::absolute(templatesRoot).string());
            return templates;
        }

        for (const auto &entry : std::filesystem::directory_iterator(templatesRoot)) {
            if (!entry.is_directory())
                continue;

            TemplateInfo info;
            info.id = entry.path().filename().string();
            info.displayName = ReadTemplateTitle(entry.path());
            if (info.displayName.empty())
                info.displayName = info.id;
            templates.push_back(std::move(info));
        }

        std::sort(templates.begin(), templates.end(), [](const TemplateInfo &a, const TemplateInfo &b) { return a.id < b.id; });
        return templates;
    }

    std::shared_ptr<Project> Project::NewProject(const std::filesystem::path &baseDir, const std::string &name, const std::string &templateId) {
        auto project = std::make_shared<Project>();
        const std::filesystem::path projectDir = baseDir / name;
        project->m_ProjectFilepath = projectDir / "project.json";

        const std::filesystem::path templateDir = PathUtils::GetExecutableDirectory() / "Templates" / templateId;
        if (!std::filesystem::exists(templateDir)) {
            LOG_ERROR(LOG_WHO, "Template directory not found at: " + std::filesystem::absolute(templateDir).string());
            return nullptr;
        }

        try {
            std::filesystem::create_directories(projectDir);
            for (const auto &entry : std::filesystem::directory_iterator(templateDir)) {
                const auto &srcPath = entry.path();
                const auto destPath = projectDir / srcPath.filename();
                std::filesystem::remove_all(destPath);
                std::filesystem::copy(srcPath, destPath, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
            }
        } catch (const std::exception &e) {
            LOG_ERROR(LOG_WHO, "Failed to copy template project: " + std::string(e.what()));
            return nullptr;
        }

        IO::VFS::MountProject(project->m_ProjectFilepath.string());

        // templates
        project->m_Config = Config::ProjectConfig::Deserialize("project.json");
        if (project->m_Config.startScenePath.empty())
            project->m_Config.startScenePath = std::string(SCENE_PATH) + "default.json";
        project->m_Config.Title = name;

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
        project->m_Config = Config::ProjectConfig::Deserialize("project.json");
        s_ActiveProject = project;
        return project;
    }

    bool Project::Save() const {
        if (!Config::ProjectConfig::Serialize(m_Config, "project.json")) {
            return false;
        }

        m_HasUnsavedChanges = false;
        return true;
    }
} // namespace Core
#pragma pop_macro("LOG_WHO")
