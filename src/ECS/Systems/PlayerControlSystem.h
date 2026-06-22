#ifndef OBLIBERRY_PLAYERCONTROLSYSTEM_H
#define OBLIBERRY_PLAYERCONTROLSYSTEM_H

#include "ECS/ECS.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Systems/DirectionalAnimationSystem.h"
#include "Math/HexMath.h"

namespace PlayerControlSystem {
    inline void Update(Registry &registry, const glm::vec2 worldPos) noexcept {
        registry.ForEach<TransformComponent, MovementComponent, MaterialComponent>(
            [&](const Entity entity, const TransformComponent *trans, const MovementComponent *move,
                const MaterialComponent *mat) {
                const glm::vec2 pPos = trans->transform.GetPosition();
                glm::vec2 targetDir;

                if (move->isMoving && move->currentPathIndex < move->currentPath.size()) {
                    const HexCoords nextHex = move->currentPath[move->currentPathIndex];
                    targetDir = Math::HexMath::HexToWorld(nextHex) - pPos;
                } else {
                    targetDir = worldPos - pPos;
                }

                DirectionalAnimation::UpdateFacing(entity, targetDir, mat);
            }
        );
    }
}

#endif //OBLIBERRY_PLAYERCONTROLSYSTEM_H
