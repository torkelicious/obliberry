#pragma once

#include "Scenes/Scene.h"
#include "Scenes/SceneManager.h"

namespace Editor::UI {
    class SceneConfigEditor {
    public:
        explicit SceneConfigEditor(Core::EngineContext *context = nullptr)
            : m_Context(context) {
        }

        void SetContext(Core::EngineContext &ctx) { m_Context = &ctx; }

        void OnImGuiRender(bool &isOpen);

        void SaveConfig();

        void ReloadFromScene();

        void ResolveMusicPath(const std::string &absolutePath);

    private:
        Core::EngineContext *m_Context = nullptr;

        // Editable copy of the current scene's properties
        Scenes::SceneProperties m_LocalProperties;
    };
} // namespace Editor::UI
