#pragma once
#include "ConfigWindow.h"
#include "Rendering/PostProcessing/PostProcessing.h"

namespace Editor::UI {

    class PostProcEditor : public ConfigEditor {
    public:
        void OnImGuiRender(bool &isOpen) override;
        void Reload() override;
        void SaveConfig() override;

    private:
        void DrawEffectList();
        void DrawEffectDetails(Rendering::PostProcessing::PostEffect &fx);
        void UniformMapToImGui(std::unordered_map<std::string, Rendering::PostProcessing::UniformValue> &uniforms);

        // live edits
        void Apply();
        void Restore();
        // push undo command
        void Commit();

        // discrete change live edit
        void NotifyInstantEdit() {
            m_Dirty = true;
            m_CommitPending = true;
        }

        std::vector<Rendering::PostProcessing::PostEffect> m_fxVecBackup; // m_OldData
        std::vector<Rendering::PostProcessing::PostEffect> m_StagingVec;  // m_NewData
        int m_SelectedFx = -1;                                            // index into m_StagingVec
        bool m_Dirty = false;
        bool m_CommitPending = false;
    };

} // namespace Editor::UI
