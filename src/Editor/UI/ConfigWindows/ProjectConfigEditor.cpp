#include "ProjectConfigEditor.h"

#include <cstring>
#include <filesystem>

#include "Core/Project.h"
#include "Core/LoggerService.h"
#include "Editor/FileDialogs.h"
#include "IO/VFS.h"
#include "imgui.h"


constexpr auto LOG_WHO = "ProjectConfigEditor";

namespace Editor::UI {
    void ProjectConfigEditor::SetContext(Core::EngineContext &context) { m_Context = &context; }

    void ProjectConfigEditor::Reload() {
        if (m_Context && m_Context->projectConfig) {
            m_LocalConfig = *m_Context->projectConfig;
            LoadConfigToBuffers();
        }
    }

    void ProjectConfigEditor::LoadConfigToBuffers() {
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

        ImGui::DragInt("Width", &m_LocalConfig.windowWidth, 1.0f, 640, 7680);
        if (ImGui::IsItemDeactivatedAfterEdit())
            Core::Project::GetActive()->MarkAsChanged();
        ImGui::DragInt("Height", &m_LocalConfig.windowHeight, 1.0f, 480, 4320);
        if (ImGui::IsItemDeactivatedAfterEdit())
            Core::Project::GetActive()->MarkAsChanged();

        if (ImGui::Checkbox("Fullscreen", &m_LocalConfig.fullscreen)) {
            Core::Project::GetActive()->MarkAsChanged();
        }

        ImGui::Separator();

        {
            ImGui::Text("Start Scene");
            ImGui::TextUnformatted(m_LocalConfig.startScenePath.empty() ? "None" : m_LocalConfig.startScenePath.c_str());
            ImGui::SameLine();
            if (ImGui::Button("Browse##Scene")) {
                if (m_Context) {
                    const auto picked = FileDialogs::OpenFile(*m_Context, {.filterName = "Scene File", .filterExt = "json"});
                    if (picked.has_value()) {
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

        // EngineContext is the source of truth
        if (m_Context && m_Context->projectConfig) {
            *m_Context->projectConfig = m_LocalConfig;
        }

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
