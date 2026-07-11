#pragma once
#include <string>

namespace Editor {
    class EditorLayer; // forward

    class EditorState {
    public:
        virtual ~EditorState() = default;

        virtual void OnEnter() {}

        virtual void OnExit() {}

        virtual void OnUpdate(float dt) = 0;

        virtual void OnHandleInput(float dt) = 0;

        virtual void OnDrawPanels() = 0;

        virtual void OnRender() = 0;

        virtual void OnDrawModeToolbar() {}

        virtual void OnDrawUtilityWindows() {}

        [[nodiscard]] virtual bool CanSaveScene() const { return true; }
        [[nodiscard]] virtual bool CanSaveSceneAs() const { return true; }
        [[nodiscard]] virtual bool IsPlayMode() const { return false; }
        [[nodiscard]] virtual const char *PlayStopLabel() const { return "Play"; }
        [[nodiscard]] virtual bool ShouldDrawProjectBrowser() const { return true; }
        [[nodiscard]] virtual const char *GetWindowTitle() const { return m_windowTitle.c_str(); } // GLFW uses C strings
        virtual void SetWindowTitle(const std::string &title) {
            if (title == m_windowTitle) {
                return;
            }
            m_windowTitle = title;
            m_windowTitleNeedsUpdate = true;
        }
        [[nodiscard]] virtual bool GetWindowTitleDirty() const { return m_windowTitleNeedsUpdate; }
        virtual void SetWindowTitleDirty(const bool dirty) { m_windowTitleNeedsUpdate = dirty; }

        virtual void SetEditorLayer(EditorLayer *layer) { m_EditorLayer = layer; }
        virtual EditorLayer *GetEditorLayer() { return m_EditorLayer; }

    protected:
        EditorLayer *m_EditorLayer = nullptr;
        std::string m_windowTitle = "Editor";
        bool m_windowTitleNeedsUpdate = true;
    };
} // namespace Editor
