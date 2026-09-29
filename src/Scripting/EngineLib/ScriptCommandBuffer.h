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

        void SetCollisionBasis(const ECS::Collision::BillboardBasis &basis);
        [[nodiscard]] const ECS::Collision::BillboardBasis &GetCollisionBasis() const;


    private:
        std::vector<SmallFunction<void(ECS::Registry &)>> m_Commands;
        std::mutex m_Mutex;
        ECS::Collision::BillboardBasis m_CollisionBasis{};
    };
} // namespace Scripting
