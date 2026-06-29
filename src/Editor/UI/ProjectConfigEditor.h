#pragma once
#include "Core/EngineContext.h"
#include "Core/ProjectConfig.h"

namespace Editor::UI {
    class ProjectConfigEditor {
        ProjectConfigEditor() = default;

        ~ProjectConfigEditor() = default;

        void SetContext(Core::EngineContext &context);

        void OnImGuiRender(bool &isOpen);

    private:
        void LoadConfigBuffers();

        void SaveConfig();

        Core::EngineContext *m_Context = nullptr;
        Core::ProjectConfig m_LocalConfig; // a working copy before saving
        bool m_IsDirty = false;
        // size buffers for ImGui input fields
        char m_TitleBuffer[256] = "";
        char m_StartSceneBuffer[512] = "";
    };
} // namespace Editor::UI
