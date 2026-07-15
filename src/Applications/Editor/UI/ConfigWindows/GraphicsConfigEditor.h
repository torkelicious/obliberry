

#pragma once
#include "ConfigWindow.h"

namespace Editor::UI {
    class GraphicsConfigEditor : public ConfigEditor {
    public:
        void Init();
        void OnImGuiRender(bool &isOpen) override;
        void Reload() override;
        void SaveConfig() override;
    private:
        void LoadConfigToBuffers();
        Config::GraphicsConfig m_LocalConfig;
        Config::GraphicsConfig m_OldConfig;
        std::vector<std::string> m_SampleLabels;
        std::vector<const char*> m_SampleLabelPtrs;
    };

} // namespace Editor::UI
