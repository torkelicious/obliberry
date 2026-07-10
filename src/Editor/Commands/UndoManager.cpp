#include "UndoManager.h"
#include "ICommand.h"
#include <deque>
#include <memory>

namespace Editor {
    UndoManager::UndoManager(const size_t maxHistory) : m_maxHistory(maxHistory) {}
    UndoManager::~UndoManager() = default;

    void UndoManager::Execute(std::unique_ptr<ICommand> command, Core::EngineContext &ctx) {
        command->Execute(ctx);
        PushUndo(std::move(command));
        m_redo.clear();
    }

    void UndoManager::Undo(Core::EngineContext &ctx) {
        if (m_undo.empty())
            return;

        auto command = std::move(m_undo.back());
        m_undo.pop_back();
        command->Undo(ctx);
        m_redo.push_back(std::move(command));
    }

    void UndoManager::Redo(Core::EngineContext &ctx) {
        if (m_redo.empty())
            return;

        auto command = std::move(m_redo.back());
        m_redo.pop_back();
        command->Execute(ctx);
        PushUndo(std::move(command));
    }

    void UndoManager::Clear() {
        m_undo.clear();
        m_redo.clear();
    }

    void UndoManager::PushUndo(std::unique_ptr<ICommand> command) {
        if (m_undo.size() >= m_maxHistory)
            m_undo.pop_front();
        m_undo.push_back(std::move(command));
    }
} // namespace Editor
