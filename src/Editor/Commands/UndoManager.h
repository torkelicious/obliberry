#pragma once

#include "Core/EngineContext.h"


#include <deque>
#include <memory>

namespace Editor {
    class ICommand;

    class UndoManager {
    public:
        explicit UndoManager(size_t maxHistory = 256);
        ~UndoManager();
        UndoManager(const UndoManager &) = delete;
        UndoManager &operator=(const UndoManager &) = delete;

        void Execute(std::unique_ptr<ICommand> command, Core::EngineContext &ctx);

        void Undo(Core::EngineContext &ctx);
        void Redo(Core::EngineContext &ctx);

        void Clear();

        [[nodiscard]]
        bool CanUndo() const noexcept {
            return !m_undo.empty();
        }

        [[nodiscard]]
        bool CanRedo() const noexcept {
            return !m_redo.empty();
        }

    private:
        void PushUndo(std::unique_ptr<ICommand> command);

    private:
        std::deque<std::unique_ptr<ICommand>> m_undo;
        std::deque<std::unique_ptr<ICommand>> m_redo;

        size_t m_maxHistory;
    };
} // namespace Editor
