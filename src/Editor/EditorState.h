#pragma once

namespace Editor {
    class EditorLayer; // forward

    class EditorState {
    public:
        virtual ~EditorState() = default;

        // basically same shit scenes / layers do

        virtual void OnEnter() {
        }

        virtual void OnExit() {
        }

        virtual void OnUpdate(float dt) = 0;

        virtual void OnHandleInput(float dt) = 0;

        virtual void OnDrawPanels() = 0;

        virtual bool CanSaveScene() const { return true; }
        virtual bool CanSaveSceneAs() const { return true; }
        virtual bool IsPlayMode() const { return false; }
        virtual const char *PlayStopLabel() const { return "Play"; }

        virtual void SetEditorLayer(EditorLayer *layer) { m_EditorLayer = layer; }
        virtual EditorLayer *GetEditorLayer() { return m_EditorLayer; }

    protected:
        EditorLayer *m_EditorLayer = nullptr;
    };
} // namespace Editor
