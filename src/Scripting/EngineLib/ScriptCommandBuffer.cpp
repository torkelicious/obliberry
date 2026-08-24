#include "ScriptCommandBuffer.h"
#include "ECS/Registry.h"

namespace Scripting {
    void ScriptCommandBuffer::push(SmallFunction<void(ECS::Registry &)> command) {
        std::lock_guard lock(m_Mutex);
        m_Commands.push_back(std::move(command));
    }

    void ScriptCommandBuffer::flush(ECS::Registry &registry) {
        std::vector<SmallFunction<void(ECS::Registry &)>> commands;
        {
            std::lock_guard lock(m_Mutex);
            commands.swap(m_Commands);
        }
        for (auto &cmd : commands) {
            cmd(registry);
        }
        commands.clear();
        {
            std::lock_guard lock(m_Mutex);
            if (m_Commands.capacity() < commands.capacity()) {
                m_Commands.swap(commands);
            }
        }
    }
} // namespace Scripting
