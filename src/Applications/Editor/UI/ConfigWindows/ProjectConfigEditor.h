#pragma once

#include "Core/EngineContext.h"
#include "Config/ProjectConfig.h"
#include "Applications/Editor/Commands/UndoManager.h"

namespace Editor::UI {
    using Commands::UndoManager;

    class ProjectConfigEditor {
    public:
        ProjectConfigEditor() = default;

        void SetContext(Core::EngineContext &context);

        void OnImGuiRender(bool &isOpen);

        void Reload();

        void SaveConfig();

        void SetUndoMgr(UndoManager *mgr) { m_Undomgr = mgr; }

    private:
        void ResolveStartScenePath(const std::string &absolutePath);

        void LoadConfigToBuffers();

        Core::EngineContext *m_Context = nullptr;
        UndoManager *m_Undomgr = nullptr;

        // Editable working copy
        Config::ProjectConfig m_LocalConfig;
        Config::ProjectConfig m_OldConfig; // copy of original for undo

        char m_TitleBuffer[256]{};
        char m_StartSceneBuffer[512]{};
    };
} // namespace Editor::UI
