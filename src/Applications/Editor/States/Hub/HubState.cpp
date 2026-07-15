#include "HubState.h"
#include "Applications/Editor/EditorLayer.h"
#include "Applications/Editor/Platform/FileDialogs.h"
#include "Applications/Editor/UI/Modals/StdDialogs.h"
#include "Core/Project.h"
#include "IO/VFS/VFS.h"
#include "Logger/LoggerService.h"
#include "imgui.h"
#include <filesystem>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "HubState"

namespace Editor::States {

    void HubState::OnEnter() { LOG_INFO(LOG_WHO, "Entering Hub State"); }

    void HubState::OnExit() { LOG_INFO(LOG_WHO, "Exiting Hub State"); }

    void HubState::OnUpdate(float /*dt*/) {}

    void HubState::OnHandleInput(float /*dt*/) {}

    void HubState::OnDrawPanels() {}

    void HubState::OnRender() {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
        ImGui::Begin("Obliberry hub", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

        ImGui::Text("welcome :)");
        ImGui::Separator();

        if (ImGui::Button("Create New Project", ImVec2(250, 50))) {
            if (const auto dir = Platform::FileDialogs::PickFolder(m_EditorLayer->m_Context)) {
                m_EditorLayer->m_NewProjectDialog.SetDirectory(std::filesystem::path(*dir));
                m_EditorLayer->m_NewProjectDialog.SetOnConfirm([this](const std::filesystem::path &pDir, const std::string &name) {
                    const auto newProject = Core::Project::NewProject(pDir, name);
                    if (newProject) {
                        m_EditorLayer->LoadProject(newProject->GetProjectPath().string());
                    }
                });
                m_EditorLayer->m_NewProjectDialog.Open();
            }
        }

        ImGui::Spacing();

        if (ImGui::Button("Open Existing Project", ImVec2(250, 50))) {
            if (const auto dir = Platform::FileDialogs::PickFolder(m_EditorLayer->m_Context)) {
                if (const std::filesystem::path projectFile = std::filesystem::path(*dir) / "project.json"; std::filesystem::exists(projectFile)) {
                    m_EditorLayer->LoadProject(projectFile.string());
                } else {
                    LOG_ERROR("Hub", "No project.json found in: " + *dir);
                }
            }
        }

        ImGui::End();
    }

} // namespace Editor::States
#pragma pop_macro("LOG_WHO")
