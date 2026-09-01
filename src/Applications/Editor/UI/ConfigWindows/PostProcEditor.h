#pragma once
#include "ConfigWindow.h"
#include "Rendering/PostProcessing/PostProcessing.h"

namespace Editor ::UI {

    class PostProcEditor : public ConfigEditor {
    public:
        void DrawFXEntry(std::vector<Rendering::PostProcessing::PostEffect> &fxVec, const std::size_t index);
        void UniformMapToImGui(std::unordered_map<std::string, Rendering::PostProcessing::UniformValue> &uniforms);
        void OnImGuiRender(bool &isOpen) override;
        void Reload() override;
        void SaveConfig() override;

        // inplace w.out undomgr
        void Apply();
        void Restore();

    private:
        std::vector<Rendering::PostProcessing::PostEffect> m_fxVecBackup; // m_OldData
        std::vector<Rendering::PostProcessing::PostEffect> m_StagingVec;  // m_LocalData
        bool m_Dirty = false;
    };

} // namespace Editor::UI
