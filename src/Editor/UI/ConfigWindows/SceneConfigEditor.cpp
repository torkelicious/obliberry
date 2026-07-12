// ReSharper disable CppDFAConstantConditions
// ReSharper disable CppDFAUnreachableCode
// why resharper is lying, i dont know :)

#include "SceneConfigEditor.h"

#include <cstring>
#include <filesystem>

#include "Core/Constants.h"
#include "Core/LoggerService.h"

#include "Editor/FileDialogs.h"
#include "Editor/UI/Panels/Editor/EditorWidgetsCombo.h"
#include "ECS/Components/MapComponent.h"
#include "IO/VFS.h"
#include "Rendering/Renderer.h"
#include "Sound/AudioEngine.h"
#include "imgui.h"
#include "Editor/Commands/EditorCommands.h"


constexpr auto LOG_WHO = "SceneConfigEditor";

namespace Editor::UI {
    void SceneConfigEditor::ReloadFromScene() {
        if (!m_Context || !m_Context->sceneManager)
            return;

        if (auto *scene = m_Context->sceneManager->GetCurrentScene()) {
            m_LocalProperties = scene->GetProperties();
        }
        m_OldProperties = m_LocalProperties;
    }

    void SceneConfigEditor::OnImGuiRender(bool &isOpen) {
        if (!isOpen)
            return;
        if (!m_Context || !m_Context->sceneManager)
            return;

        if (m_LocalProperties.Name.empty()) {
            ReloadFromScene();
        }

        if (!m_Context->sceneManager->GetCurrentScene()) {
            isOpen = false;
            return;
        }

        ImGui::Begin("Scene Properties", &isOpen);

        char nameBuf[256]{};
        std::strncpy(nameBuf, m_LocalProperties.Name.c_str(), sizeof(nameBuf) - 1);
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
            m_LocalProperties.Name = nameBuf;
        }

        ImGui::Text("Path: %s", m_LocalProperties.ScenePath.c_str());

        ImGui::Separator();

        ImGui::ColorEdit4("Clear Color", &m_LocalProperties.BackgroundClearColor.x, ImGuiColorEditFlags_AlphaBar);

        ImGui::SliderFloat("Ambient Light", &m_LocalProperties.AmbientLight, 0.0f, 1.0f, "%.3f");

        ImGui::Separator();

        {
            ImGui::Text("Background Music");
            ImGui::TextUnformatted(m_LocalProperties.BackgroundMusicPath.empty() ? "None" : m_LocalProperties.BackgroundMusicPath.c_str());
            ImGui::SameLine();
            if (ImGui::Button("Load##Music")) {
                if (m_Context) {
                    if (const auto picked = FileDialogs::OpenFile(*m_Context, {.filterName = "Audio Files", .filterExt = "wav,mp3,ogg,flac"}); picked.has_value()) {
                        ResolveMusicPath(picked.value());
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear##Music")) {
                m_LocalProperties.BackgroundMusicPath.clear();
            }
        }

        ImGui::Separator();

        if (auto *scene = m_Context->sceneManager->GetCurrentScene()) {
            scene->GetRegistry().ForEach<ECS::Components::MapComponent>([&](ECS::Entity, ECS::Components::MapComponent *mapComp) {
                ImGui::SeparatorText("Map");
                ImGui::PushID("SceneMapFileCombo");
                if (FileCombo("Map File", std::string(Core::MAP_PATH), std::string(Core::MAP_FILE_EXTENSION), mapComp->mapFilePath)) {
                    mapComp->mapDirty = true;
                    scene->MarkAsChanged();
                }
                ImGui::PopID();
            });
        }

        ImGui::SeparatorText("");

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
            ReloadFromScene();
            isOpen = false;
        }

        ImGui::End();
    }

    void SceneConfigEditor::SaveConfig() {
        if (!m_Context || !m_Context->sceneManager)
            return;

        auto *scene = m_Context->sceneManager->GetCurrentScene();
        if (!scene)
            return;

        m_Undomgr->Execute(std::make_unique<Commands::UpdateScenePropertiesCommand>(m_OldProperties, m_LocalProperties), *m_Context);
        m_OldProperties = m_LocalProperties;

        scene->MarkAsChanged();

        LOG_INFO(LOG_WHO, "Scene properties saved");
    }

    void SceneConfigEditor::ResolveMusicPath(const std::string &absolutePath) {
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
                m_LocalProperties.BackgroundMusicPath = rel;
                return;
            }
        }
        m_LocalProperties.BackgroundMusicPath = absolutePath;
    }
} // namespace Editor::UI
