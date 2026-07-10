#pragma once

#include <deque>
#include <memory>
#include <cstddef>

namespace Editor {
    class ICommand;

    class UndoManager {
    public:
        explicit UndoManager(const size_t maxHistory = 256);
        ~UndoManager();
        UndoManager(const UndoManager &) = delete;
        UndoManager &operator=(const UndoManager &) = delete;

        void Execute(std::unique_ptr<ICommand> command);

        void Undo();
        void Redo();

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
