#pragma once

#include "ECS/Systems/Collision/ColliderGeometry.h"
#include "Scripting/SmallFunction.h"
#include <mutex>
#include <vector>

namespace ECS {
    class Registry;
}

namespace Scripting {
    // safe buffer of deferred registry mutations because thread safety yay
    class ScriptCommandBuffer {
    public:
        void push(SmallFunction<void(ECS::Registry &)> command);
        void flush(ECS::Registry &registry);
        // while immediate mode is active, push() applies each command synchronously instead of deferring it
        // used for single-threaded execution
        // not thread-safe
        void BeginImmediate(ECS::Registry &registry);
        void EndImmediate();

        void SetCollisionBasis(const ECS::Collision::BillboardBasis &basis);
        [[nodiscard]] const ECS::Collision::BillboardBasis &GetCollisionBasis() const;


    private:
        std::vector<SmallFunction<void(ECS::Registry &)>> m_Commands;
        std::mutex m_Mutex;
        ECS::Collision::BillboardBasis m_CollisionBasis{};
        ECS::Registry *m_ImmediateRegistry = nullptr;
    };
} // namespace Scripting
