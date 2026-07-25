#pragma once
#include <string>

namespace Editor {
    class EditorLayer;
}

namespace Editor::States {
    class EditorStateBase {
    public:
        virtual ~EditorStateBase() = default;

        virtual void OnEnter() {}

        virtual void OnExit() {}

        virtual void OnSceneLoaded() {}

        virtual void OnUpdate(float dt) = 0;

        virtual void OnHandleInput(float dt) = 0;

        virtual void OnDrawPanels() = 0;

        virtual void OnRender() = 0;

        virtual void OnDrawModeToolbar() {}

        virtual void OnDrawUtilityWindows() {}

        virtual void OnSaveKey() {}

        [[nodiscard]] virtual bool CanSaveScene() const { return true; }
        [[nodiscard]] virtual bool CanSaveSceneAs() const { return true; }
        [[nodiscard]] virtual bool IsPlayMode() const { return false; }
        [[nodiscard]] virtual const char *PlayStopLabel() const { return "Play"; }
        [[nodiscard]] virtual bool ShouldDrawProjectBrowser() const { return true; }
        [[nodiscard]] const char *GetWindowTitle() const { return m_windowTitle.c_str(); } // GLFW uses C strings
        void SetWindowTitle(const std::string &title) {
            if (title == m_windowTitle) {
                return;
            }
            m_windowTitle = title;
            m_windowTitleNeedsUpdate = true;
        }
        [[nodiscard]] bool GetWindowTitleDirty() const { return m_windowTitleNeedsUpdate; }
        void SetWindowTitleDirty(const bool dirty) { m_windowTitleNeedsUpdate = dirty; }

        void SetEditorLayer(EditorLayer *layer) { m_EditorLayer = layer; }
        [[nodiscard]] EditorLayer *GetEditorLayer() const { return m_EditorLayer; }

    protected:
        EditorLayer *m_EditorLayer = nullptr;
        std::string m_windowTitle = "Editor";
        bool m_windowTitleNeedsUpdate = true;
    };
} // namespace Editor::States
