#include "ScriptCommandBuffer.h"
#include "ECS/Registry.h"
#include "ECS/Systems/Collision/ColliderGeometry.h"

namespace Scripting {
    void ScriptCommandBuffer::push(SmallFunction<void(ECS::Registry &)> command) {
        if (m_ImmediateRegistry) {
            command(*m_ImmediateRegistry);
            return;
        }
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

    void ScriptCommandBuffer::BeginImmediate(ECS::Registry &registry) { m_ImmediateRegistry = &registry; }

    void ScriptCommandBuffer::EndImmediate() { m_ImmediateRegistry = nullptr; }

    void ScriptCommandBuffer::SetCollisionBasis(const ECS::Collision::BillboardBasis &basis) { m_CollisionBasis = basis; }

    const ECS::Collision::BillboardBasis &ScriptCommandBuffer::GetCollisionBasis() const { return m_CollisionBasis; }


} // namespace Scripting
