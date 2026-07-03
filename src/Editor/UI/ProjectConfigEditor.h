#pragma once

#include "Core/EngineContext.h"
#include "Core/ProjectConfig.h"

namespace Editor::UI {
    class ProjectConfigEditor {
    public:
        ProjectConfigEditor() = default;

        void SetContext(Core::EngineContext &context);

        void OnImGuiRender(bool &isOpen);

        void Reload();

        void SaveConfig();

    private:
        void ResolveStartScenePath(const std::string &absolutePath);

        void LoadConfigToBuffers();

        Core::EngineContext *m_Context = nullptr;

        // Editable working copy
        Core::ProjectConfig m_LocalConfig;

        char m_TitleBuffer[256]{};
        char m_StartSceneBuffer[512]{};
    };
} // namespace Editor::UI
