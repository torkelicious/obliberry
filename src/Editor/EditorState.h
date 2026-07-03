#pragma once

namespace Editor {
    class EditorLayer; // forward

    class EditorState {
    public:
        virtual ~EditorState() = default;

        // basically same shit scenes / layers do

        virtual void OnEnter(EditorLayer &editor) {
        }

        virtual void OnExit(EditorLayer &editor) {
        }

        virtual void OnUpdate(EditorLayer &editor, float dt) = 0;

        virtual void OnHandleInput(EditorLayer &editor, float dt) = 0;

        virtual void OnDrawPanels(EditorLayer &editor) = 0;

        virtual bool CanSaveScene() const { return true; }
        virtual bool CanSaveSceneAs() const { return true; }
        virtual bool IsPlayMode() const { return false; }
        virtual const char *PlayStopLabel() const { return "Play"; }
    };
} // namespace Editor
