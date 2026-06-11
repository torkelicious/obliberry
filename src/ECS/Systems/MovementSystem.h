#ifndef OBLIBERRY_MOVEMENT_H
#define OBLIBERRY_MOVEMENT_H

#include <vector>
#include <utility>
#include <glm/glm.hpp>
#include "ECS/ECS.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MapComponent.h"

namespace MovementSystem {
    inline void CancelPath(MovementComponent *moveComp) {
        if (!moveComp) return;
        moveComp->isMoving = false;
        moveComp->currentPathIndex = 0;
        moveComp->stepTimer = 0.0f;
        moveComp->currentPath.clear();
    }

    inline void SetPath(const Entity entity, std::vector<HexCoords> newPath) {
        auto *moveComp = entity.GetComponent<MovementComponent>();
        if (!moveComp) return;

        if (newPath.empty()) {
            CancelPath(moveComp);
            return;
        }

        moveComp->currentPath = std::move(newPath);
        moveComp->currentPathIndex = 0;
        moveComp->stepTimer = 0.0f;
        moveComp->isMoving = true;
    }

    inline void Update(Registry &registry, const float dt) {
        const MapComponent *map = nullptr;
        registry.ForEach<MapComponent>([&](Entity, const MapComponent *m) { map = m; });

        if (!map) return;

        registry.ForEach<MovementComponent, TransformComponent>(
            [&](Entity entity, MovementComponent *moveComp, TransformComponent *transComp) {
                if (!moveComp->isMoving) return;

                if (moveComp->currentPathIndex >= moveComp->currentPath.size()) {
                    CancelPath(moveComp);
                    return;
                }

                moveComp->stepTimer += dt;
                if (moveComp->stepTimer >= moveComp->timePerStep) {
                    moveComp->stepTimer -= moveComp->timePerStep;
                    const HexCoords targetHex = moveComp->currentPath[moveComp->currentPathIndex];

                    const glm::vec2 targetWorldPos2D = HexGrid::GetWorldPos(targetHex);

                    transComp->transform.SetPosition(glm::vec3(
                        targetWorldPos2D.x,
                        targetWorldPos2D.y,
                        transComp->transform.GetPosition().z
                    ));

                    moveComp->currentPathIndex++;
                    if (moveComp->currentPathIndex >= moveComp->currentPath.size()) {
                        CancelPath(moveComp);
                    }
                }
            }
        );
    }

    inline void MoveToCenter(const Entity entity) {
        auto *moveComp = entity.GetComponent<MovementComponent>();
        auto *transComp = entity.GetComponent<TransformComponent>();

        if (!moveComp || !transComp) return;

        CancelPath(moveComp);

        transComp->transform.SetPosition(glm::vec3(
            0.0f, 0.0f, transComp->transform.GetPosition().z
        ));
    }
}

#endif //OBLIBERRY_MOVEMENT_H
