#pragma once

#include "Applications/Editor/Commands/UndoManager.h"
#include "Scenes/Scene.h"
#include "Scenes/SceneManager.h"

namespace Editor::UI {
    using Commands::UndoManager;

    class SceneConfigEditor : public ConfigEditor {
    public:
        void OnImGuiRender(bool &isOpen) override;

        void SaveConfig() override;

        void ReloadFromScene();

        void ResolveMusicPath(const std::string &absolutePath);

    private:
        // Editable copy of the current scene's properties
        Scenes::SceneProperties m_LocalProperties;

        Scenes::SceneProperties m_OldProperties;
    };
} // namespace Editor::UI
