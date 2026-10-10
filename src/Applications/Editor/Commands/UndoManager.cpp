#include "UndoManager.h"
#include "Core/EngineContext.h"
#include "ICommand.h"
#include "Logger/LoggerService.h"
#include <deque>
#include <exception>
#include <memory>

namespace Editor::Commands {

    namespace {
        bool ApplyCommand(ICommand &cmd, Core::EngineContext &ctx, const bool undo) {
            try {
                if (undo) {
                    cmd.Undo(ctx);
                } else {
                    cmd.Execute(ctx);
                }
                return cmd.Succeeded();
            } catch (const std::exception &e) {
                LOG_ERROR("UndoManager", std::string(cmd.Name()) + ": " + e.what());
                return false;
            }
        }

    } // namespace

    UndoManager::UndoManager(const size_t maxHistory) : m_maxHistory(maxHistory) {}

    UndoManager::~UndoManager() = default;

    void UndoManager::Execute(std::unique_ptr<ICommand> command, Core::EngineContext &ctx) {
        if (!command || !ApplyCommand(*command, ctx, false)) {
            return;
        }

        PushUndo(std::move(command));
        m_redo.clear();
    }

    void UndoManager::Undo(Core::EngineContext &ctx) {
        if (m_undo.empty() || !ApplyCommand(*m_undo.back(), ctx, true)) {
            return;
        }
        m_redo.push_back(std::move(m_undo.back()));
        m_undo.pop_back();
    }

    void UndoManager::Redo(Core::EngineContext &ctx) {
        if (m_redo.empty() || !ApplyCommand(*m_redo.back(), ctx, false)) {
            return;
        }
        PushUndo(std::move(m_redo.back()));
        m_redo.pop_back();
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
} // namespace Editor::Commands
