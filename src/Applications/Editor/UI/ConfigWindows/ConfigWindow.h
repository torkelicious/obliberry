#pragma once
#pragma once
#include "Core/EngineContext.h"
#include "Config/ProjectConfig.h"
#include "Applications/Editor/Commands/UndoManager.h"

namespace Editor::UI {
    using Commands::UndoManager;
    class ConfigEditor {
    public:
        virtual ~ConfigEditor() = default;

        virtual void OnImGuiRender(bool &isOpen);

        virtual void Reload();

        virtual void SaveConfig();

        void SetContext(Core::EngineContext &context) { m_Context = &context; }

        void SetUndoMgr(UndoManager *mgr) { m_Undomgr = mgr; }

    protected:
        Core::EngineContext *m_Context = nullptr;
        UndoManager *m_Undomgr = nullptr;
    };
} // namespace Editor::UI
