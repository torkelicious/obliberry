#include "ProjectConfigEditor.h"

#include <cstring>
#include <filesystem>
#include "Core/Project.h"
#include "Logger/LoggerService.h"
#include "Applications/Editor/Platform/FileDialogs.h"
#include "IO/VFS/VFS.h"
#include "imgui.h"
#include "Platform/Window/Window.h"
#include "Applications/Editor/Commands/EditorCommands.h"


#pragma push_macro("LOG_WHO")
#define LOG_WHO "ProjectConfigEditor"

namespace Editor::UI {
    void ProjectConfigEditor::Reload() {
        if (m_Context && m_Context->projectConfig) {
            m_LocalConfig = *m_Context->projectConfig;
            LoadConfigToBuffers();
        }
    }

    void ProjectConfigEditor::LoadConfigToBuffers() {
        m_OldConfig = m_LocalConfig;
        std::strncpy(m_TitleBuffer, m_LocalConfig.Title.c_str(), sizeof(m_TitleBuffer) - 1);
        std::strncpy(m_StartSceneBuffer, m_LocalConfig.startScenePath.c_str(), sizeof(m_StartSceneBuffer) - 1);
    }

    void ProjectConfigEditor::OnImGuiRender(bool &isOpen) {
        if (!isOpen)
            return;
        if (!m_Context)
            return;

        if (m_TitleBuffer[0] == '\0' && !m_LocalConfig.Title.empty()) {
            LoadConfigToBuffers();
        }

        // auto close if there is no project
        if (!Core::Project::GetActive()) {
            isOpen = false;
            return;
        }

        ImGui::Begin("Project Properties", &isOpen);

        // title
        if (ImGui::InputText("Window Title", m_TitleBuffer, sizeof(m_TitleBuffer))) {
            m_LocalConfig.Title = m_TitleBuffer;
            Core::Project::GetActive()->MarkAsChanged();
        }

        ImGui::Separator();

        {
            ImGui::Text("Start Scene");
            ImGui::TextUnformatted(m_LocalConfig.startScenePath.empty() ? "None" : m_LocalConfig.startScenePath.c_str());
            ImGui::SameLine();
            if (ImGui::Button("Browse##Scene")) {
                if (m_Context) {
                    if (const auto picked = Platform::FileDialogs::OpenFile(*m_Context, {.filterName = "Scene File", .filterExt = "json"}); picked.has_value()) {
                        ResolveStartScenePath(picked.value());
                        Core::Project::GetActive()->MarkAsChanged();
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear##Scene")) {
                m_LocalConfig.startScenePath.clear();
                m_StartSceneBuffer[0] = '\0';
                Core::Project::GetActive()->MarkAsChanged();
            }
        }

        ImGui::SeparatorText("");

        // buttons
        constexpr float buttonWidth = 120.0f;

        if (ImGui::Button("Save", ImVec2(buttonWidth, 0.0f))) {
            SaveConfig();
            isOpen = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply", ImVec2(buttonWidth, 0.0f))) {
            SaveConfig();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f))) {
            Reload();
            isOpen = false;
        }

        ImGui::End();
    }

    void ProjectConfigEditor::SaveConfig() {
        const auto project = Core::Project::GetActive();
        if (!project)
            return;

        // sync
        m_LocalConfig.Title = m_TitleBuffer;
        m_LocalConfig.startScenePath = m_StartSceneBuffer;

        m_Undomgr->Execute(std::make_unique<Commands::ProjectConfigUpdateCommand>(m_OldConfig, m_LocalConfig), *m_Context);
        m_OldConfig = m_LocalConfig;

        // write to disk via Project
        project->GetConfig() = m_LocalConfig;
        if (project->Save()) {
            project->ClearUnsavedChanges();
            LOG_INFO(LOG_WHO, "Project properties saved");
        } else {
            LOG_ERROR(LOG_WHO, "Failed to save project properties");
        }
    }

    void ProjectConfigEditor::ResolveStartScenePath(const std::string &absolutePath) {
        if (absolutePath.empty())
            return;

        const std::filesystem::path projectRoot = IO::VFS::GetProjectRoot();
        if (projectRoot.empty())
            return;

        std::error_code ec;
        std::filesystem::path absNorm = std::filesystem::absolute(absolutePath, ec);
        if (ec || absNorm.empty())
            return;
        absNorm = absNorm.lexically_normal();

        std::filesystem::path rootNorm = std::filesystem::absolute(projectRoot, ec);
        if (ec || rootNorm.empty())
            return;
        rootNorm = rootNorm.lexically_normal();

        // check if the file is inside the project root
        for (auto p = absNorm; p.has_parent_path() && p != p.root_path(); p = p.parent_path()) {
            if (p == rootNorm) {
                std::string rel = std::filesystem::proximate(absNorm, rootNorm).string();
                for (auto &c : rel)
                    if (c == '\\')
                        c = '/';
                m_LocalConfig.startScenePath = rel;
                std::strncpy(m_StartSceneBuffer, rel.c_str(), sizeof(m_StartSceneBuffer) - 1);
                return;
            }
        }
        m_LocalConfig.startScenePath = absolutePath;
        std::strncpy(m_StartSceneBuffer, absolutePath.c_str(), sizeof(m_StartSceneBuffer) - 1);
    }
} // namespace Editor::UI
#pragma pop_macro("LOG_WHO")
