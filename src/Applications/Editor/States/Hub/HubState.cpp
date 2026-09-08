#include "HubState.h"
#include "Applications/Editor/EditorLayer.h"
#include "Applications/Editor/Platform/FileDialogs.h"
#include "Applications/Editor/States/Hub/ProjectHistory.h"
#include "Applications/Editor/UI/Modals/StdDialogs.h"
#include "Core/Project.h"
#include "Logger/LoggerService.h"
#include "imgui.h"
#include "imgui_internal.h" // needed for DockBuilder* (default hub layout)
#include <filesystem>
#include "Core/Utils/FmtUtils.h"

#pragma push_macro("LOG_WHO")
#define LOG_WHO "HubState"

namespace Editor::States {

    namespace {
        std::string TruncatePathToWidth(const std::string &path, float availWidth) {
            if (path.empty() || availWidth <= 0.0f)
                return path;
            if (ImGui::CalcTextSize(path.c_str()).x <= availWidth)
                return path;

            const std::string ellipsis = "...";
            const float ellipsisWidth = ImGui::CalcTextSize(ellipsis.c_str()).x;
            if (availWidth <= ellipsisWidth)
                return ellipsis;

            size_t low = 1, high = path.length();
            std::string bestFit = ellipsis;

            while (low <= high) {
                size_t mid = low + (high - low) / 2;
                size_t keepStart = mid / 2;
                size_t keepEnd = mid - keepStart;

                if (keepStart + keepEnd >= path.length())
                    break;

                std::string test = path.substr(0, keepStart) + ellipsis + path.substr(path.length() - keepEnd);
                if (ImGui::CalcTextSize(test.c_str()).x <= availWidth) {
                    bestFit = test;
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            }
            return bestFit;
        }
    } // namespace

    std::optional<size_t> HubState::DrawProjectEntry(const size_t index, const ProjectHistoryEntry &entry) const {
        std::optional<size_t> removeRequested;

        ImGui::PushID(static_cast<int>(index));

        const float cardWidth = ImGui::GetContentRegionAvail().x;
        const ImVec2 startPos = ImGui::GetCursorScreenPos();
        ImDrawList *drawList = ImGui::GetWindowDrawList();

        const std::string fullPath = entry.filePath.string();
        const std::string dateStr = Core::Utils::Fmt::formatFileTimestamp(entry.timestamp);

        const float padding = 10.0f;
        const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
        const float cardHeight = (lineHeight * 3) + (padding * 2);

        const bool clicked = ImGui::InvisibleButton("##card_hitbox", ImVec2(cardWidth, cardHeight));
        const bool isHovered = ImGui::IsItemHovered();
        ImGui::SetNextItemAllowOverlap();

        if (isHovered) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        if (clicked) {
            m_EditorLayer->LoadProject(entry.filePath.string());
        }

        // card background
        const ImU32 bgColor = isHovered ? ImGui::GetColorU32(ImGuiCol_HeaderHovered) : ImGui::GetColorU32(ImGuiCol_FrameBg);
        const ImU32 borderColor = ImGui::GetColorU32(ImGuiCol_Border);

        drawList->AddRectFilled(startPos, ImVec2(startPos.x + cardWidth, startPos.y + cardHeight), bgColor, 4.0f);
        drawList->AddRect(startPos, ImVec2(startPos.x + cardWidth, startPos.y + cardHeight), borderColor, 4.0f);

        // close button
        const float removeBtnSize = ImGui::GetFrameHeight();
        const float contentAvailWidth = cardWidth - (padding * 3) - removeBtnSize;

        // card text overtop background
        ImGui::SetCursorScreenPos(ImVec2(startPos.x + padding, startPos.y + padding));
        ImGui::BeginGroup();

        const std::string truncatedTitle = TruncatePathToWidth(entry.ProjectConfig.Title, contentAvailWidth);
        ImGui::TextUnformatted(truncatedTitle.c_str());
        if (ImGui::IsItemHovered() && truncatedTitle != entry.ProjectConfig.Title) {
            ImGui::SetTooltip("%s", entry.ProjectConfig.Title.c_str());
        }

        ImGui::TextDisabled("Last Modified: %s", dateStr.c_str());

        const std::string truncatedPath = TruncatePathToWidth(fullPath, contentAvailWidth);
        ImGui::TextDisabled("Path: %s", truncatedPath.c_str());
        if (ImGui::IsItemHovered() && truncatedPath != fullPath) {
            ImGui::SetTooltip("%s", fullPath.c_str());
        }

        ImGui::EndGroup();

        // position close button
        ImGui::SetCursorScreenPos(ImVec2(startPos.x + cardWidth - padding - removeBtnSize, startPos.y + padding));
        if (!isHovered) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::Button("X", ImVec2(removeBtnSize, removeBtnSize))) {
            removeRequested = index;
        }
        if (!isHovered) {
            ImGui::PopStyleVar();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Remove from recent projects");
        }

        // the bounding box of the card
        ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + cardHeight));
        ImGui::Dummy(ImVec2(cardWidth, 8.0f));

        ImGui::PopID();
        return removeRequested;
    }

    void HubState::OnEnter() { LOG_INFO(LOG_WHO, "Entering Hub State"); }

    void HubState::OnExit() { LOG_INFO(LOG_WHO, "Exiting Hub State"); }

    void HubState::OnUpdate(float /*dt*/) {}

    void HubState::OnHandleInput(float /*dt*/) {}

    void HubState::OnDrawPanels() {}

    void HubState::OnRender() {
        // invisible host window that covers the entire viewport to host the DockSpace
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                     ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("HubDockHost", nullptr, hostFlags);
        ImGui::PopStyleVar(3);

        // submit the DockSpace
        ImGuiID dockspaceId = ImGui::GetID("HubDockSpace");
        const ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar;
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

        {
            static bool layoutInitialized = false;
            if (!layoutInitialized) {
                layoutInitialized = true;
                if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr) {
                    ImGui::DockBuilderRemoveNode(dockspaceId);
                    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
                    ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

                    ImGuiID leftId, rightId;
                    ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.28f, &leftId, &rightId);

                    ImGui::DockBuilderDockWindow("Actions", leftId);
                    ImGui::DockBuilderDockWindow("Recent Projects", rightId);

                    ImGui::DockBuilderFinish(dockspaceId);
                }
            }
        }

        ImGui::Begin("Actions");
        ImGui::Spacing();
        ImGui::TextUnformatted("Obliberry Hub");
        ImGui::TextDisabled("welcome :D");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Create New Project", ImVec2(-1.0f, 45.0f))) {
            if (const auto dir = Platform::FileDialogs::PickFolder((*m_EditorLayer->m_Context))) {
                m_EditorLayer->m_NewProjectDialog.SetDirectory(std::filesystem::path(*dir));
                m_EditorLayer->m_NewProjectDialog.SetTemplates(Core::Project::GetAvailableTemplates());
                m_EditorLayer->m_NewProjectDialog.SetOnConfirm([this](const std::filesystem::path &pDir, const std::string &name, const std::string &templateId) {
                    if (const auto newProject = Core::Project::NewProject(pDir, name, templateId)) {
                        m_EditorLayer->LoadProject(newProject->GetProjectPath().string());
                    }
                });
                m_EditorLayer->m_NewProjectDialog.Open();
            }
        }

        ImGui::Spacing();

        if (ImGui::Button("Open Existing Project", ImVec2(-1.0f, 45.0f))) {
            if (const auto dir = Platform::FileDialogs::PickFolder((*m_EditorLayer->m_Context))) {
                if (const std::filesystem::path projectFile = std::filesystem::path(*dir) / "project.json"; std::filesystem::exists(projectFile)) {
                    m_EditorLayer->LoadProject(projectFile.string());
                } else {
                    LOG_ERROR("Hub", "No project.json found in: " + *dir);
                    m_OpenProjectError = *dir;
                    ImGui::OpenPopup("Project Not Found");
                }
            }
        }

        if (ImGui::BeginPopupModal("Project Not Found", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("No project.json was found in:");
            ImGui::TextWrapped("%s", m_OpenProjectError.c_str());
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::End();

        // recents
        ImGui::Begin("Recent Projects");

        ImGui::Spacing();
        if (m_EditorLayer->m_ProjectHistory.empty()) {
            ImGui::TextDisabled("No recent projects found. Create or open a project to get started.");
        } else {
            std::optional<size_t> removeRequested;
            for (size_t i = 0; i < m_EditorLayer->m_ProjectHistory.size(); ++i) {
                if (const auto result = DrawProjectEntry(i, m_EditorLayer->m_ProjectHistory[i])) {
                    removeRequested = result;
                }
            }

            if (removeRequested) {
                m_EditorLayer->m_ProjectHistory.remove_at(*removeRequested);
            }
        }

        ImGui::End();
        ImGui::End(); // HubDockHost
    }

} // namespace Editor::States
#pragma pop_macro("LOG_WHO")
