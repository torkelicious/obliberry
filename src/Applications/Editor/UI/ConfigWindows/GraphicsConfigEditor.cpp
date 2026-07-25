#include "GraphicsConfigEditor.h"
#include "Applications/Editor/Commands/EditorCommands.h"
#include "IO/VFS/VFS.h"
#include "imgui.h"
#include <algorithm>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "GraphicsConfigEditor"

namespace Editor::UI {

    void GraphicsConfigEditor::Init() {
        m_SampleLabels.clear();
        m_SampleLabelPtrs.clear();
        for (uint8_t s : Config::GraphicsCapabilities::s_SupportedSampleCounts)
            m_SampleLabels.push_back(std::to_string(s));
        for (auto &label : m_SampleLabels)
            m_SampleLabelPtrs.push_back(label.c_str());

        m_LocalConfig.AASamples = Config::GraphicsConfig::SnapToValidSampleCount(m_LocalConfig.AASamples, Config::GraphicsCapabilities::s_SupportedSampleCounts);
    }

    void GraphicsConfigEditor::OnImGuiRender(bool &isOpen) {
        if (!isOpen)
            return;
        if (!m_Context)
            return;

        ImGui::Begin("Project Graphics Settings", &isOpen);

        // Window
        ImGui::SeparatorText("Window");
        ImGui::SetNextItemWidth(80.0f);
        ImGui::InputInt("##Width", &m_LocalConfig.WindowWidth, 0, 0);
        ImGui::SameLine();
        ImGui::TextDisabled("x");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        ImGui::InputInt("##Height", &m_LocalConfig.WindowHeight, 0, 0);
        ImGui::SameLine();
        ImGui::TextDisabled("(W x H)");
        m_LocalConfig.WindowWidth = std::max(m_LocalConfig.WindowWidth, 1);
        m_LocalConfig.WindowHeight = std::max(m_LocalConfig.WindowHeight, 1);

        // Frame rate
        ImGui::SeparatorText("Frame Rate");
        ImGui::DragInt("Target FPS", &m_LocalConfig.TargetFPS, 1.0f, 0, 10000, "%d FPS");
        if (m_LocalConfig.VSync != Config::VSyncType::NONE) {
            ImGui::TextDisabled("(ignored when VSync is enabled)");
        }

        // VSync
        ImGui::SeparatorText("VSync");
        const char *vsyncTypes[] = {"None", "Enabled", "Adaptive"};
        int currentVsyncType = static_cast<int>(m_LocalConfig.VSync);
        if (ImGui::Combo("Mode", &currentVsyncType, vsyncTypes, IM_ARRAYSIZE(vsyncTypes))) {
            m_LocalConfig.VSync = static_cast<Config::VSyncType>(currentVsyncType);
        }

        // Antialiasing
        ImGui::SeparatorText("Anti-Aliasing");
        ImGui::Checkbox("MSAA Enabled", &m_LocalConfig.MSAAEnabled);
        if (m_LocalConfig.MSAAEnabled) {
            ImGui::Indent();
            int currentIndex = 0;
            for (size_t i = 0; i < Config::GraphicsCapabilities::s_SupportedSampleCounts.size(); ++i) {
                if (Config::GraphicsCapabilities::s_SupportedSampleCounts[i] == m_LocalConfig.AASamples) {
                    currentIndex = static_cast<int>(i);
                    break;
                }
            }
            if (ImGui::Combo("Samples", &currentIndex, m_SampleLabelPtrs.data(), static_cast<int>(m_SampleLabelPtrs.size()))) {
                m_LocalConfig.AASamples = Config::GraphicsCapabilities::s_SupportedSampleCounts[currentIndex];
            }
            ImGui::Unindent();
        }

        // Buttons
        ImGui::SeparatorText("");
        constexpr float buttonWidth = 120.0f;
        if (ImGui::Button("Save", ImVec2(buttonWidth, 0.0f))) {
            m_Undomgr->Execute(std::make_unique<Commands::GraphicsConfigUpdateCommand>(m_OldConfig, m_LocalConfig), *m_Context);
            Config::GraphicsConfig::Serialize(m_LocalConfig, IO::VFS::GetProjectRoot() / "graphics.json");
            isOpen = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f))) {
            Reload();
            isOpen = false;
        }
        ImGui::TextDisabled("Some settings only apply in runtime");


        ImGui::End();
    }

    void GraphicsConfigEditor::Reload() {
        if (m_Context && m_Context->graphicsConfig) {
            m_LocalConfig = *m_Context->graphicsConfig;
            LoadConfigToBuffers();
        }
    }

    void GraphicsConfigEditor::SaveConfig() {}
    void GraphicsConfigEditor::LoadConfigToBuffers() { m_OldConfig = m_LocalConfig; }


} // namespace Editor::UI

#pragma pop_macro("LOG_WHO")
