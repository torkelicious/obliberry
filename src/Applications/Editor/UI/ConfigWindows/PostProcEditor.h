#pragma once
#include "ConfigWindow.h"
#include "Rendering/PostProcessing/PostProcessing.h"

namespace Editor ::UI {

    class PostProcEditor : public ConfigEditor {
    public:
        void OnImGuiRender(bool &isOpen) override;
        void Reload() override;
        void SaveConfig() override;

    private:
        std::vector<Rendering::PostProcessing::PostEffect> m_fxVecBackup; // m_OldData
        std::vector<Rendering::PostProcessing::PostEffect> m_StagingVec;  // m_LocalData
    };

} // namespace Editor::UI
