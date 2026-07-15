#pragma once

#include "Applications/Editor/Commands/UndoManager.h"
#include "Scenes/Scene.h"
#include "Scenes/SceneManager.h"

namespace Editor::UI {
    using Commands::UndoManager;

    class SceneConfigEditor {
    public:
        explicit SceneConfigEditor(Core::EngineContext *context = nullptr) : m_Context(context) {}

        void SetContext(Core::EngineContext &ctx) { m_Context = &ctx; }

        void OnImGuiRender(bool &isOpen);

        void SaveConfig();

        void ReloadFromScene();

        void ResolveMusicPath(const std::string &absolutePath);

        void SetUndoMgr(UndoManager *mgr) { m_Undomgr = mgr; }

    private:
        Core::EngineContext *m_Context = nullptr;

        UndoManager *m_Undomgr = nullptr;
        // Editable copy of the current scene's properties
        Scenes::SceneProperties m_LocalProperties;

        Scenes::SceneProperties m_OldProperties;
    };
} // namespace Editor::UI
