#include "ScriptCommandBuffer.h"
#include "ECS/Registry.h"

namespace Scripting {
    void ScriptCommandBuffer::push(std::function<void(ECS::Registry &)> command) {
        std::lock_guard lock(m_Mutex);
        m_Commands.push_back(std::move(command));
    }

    void ScriptCommandBuffer::flush(ECS::Registry &registry) {
        // Swap
        std::vector<std::function<void(ECS::Registry &)>> commands;
        {
            std::lock_guard lock(m_Mutex);
            commands.swap(m_Commands);
        }
        for (auto &cmd : commands) {
            cmd(registry);
        }
    }
} // namespace Scripting
