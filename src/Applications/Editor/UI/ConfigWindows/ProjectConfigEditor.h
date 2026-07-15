#pragma once

#include "ConfigWindow.h"
#include "Core/EngineContext.h"
#include "Config/ProjectConfig.h"
#include "Applications/Editor/Commands/UndoManager.h"

namespace Editor::UI {
    using Commands::UndoManager;

    class ProjectConfigEditor : public ConfigEditor {
    public:
        void OnImGuiRender(bool &isOpen) override;

        void Reload() override;

        void SaveConfig() override;

    private:
        void ResolveStartScenePath(const std::string &absolutePath);

        void LoadConfigToBuffers();

        // Editable working copy
        Config::ProjectConfig m_LocalConfig;
        Config::ProjectConfig m_OldConfig; // copy of original for undo

        char m_TitleBuffer[256]{};
        char m_StartSceneBuffer[512]{};
    };
} // namespace Editor::UI
