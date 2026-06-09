#ifndef OBLIBERRY_MOVEMENT_H
#define OBLIBERRY_MOVEMENT_H

#include <vector>
#include <span>
#include <utility>
#include <glm/glm.hpp>
#include "ECS/ECS.h"
#include "Map/Hex.h"
#include "ECS/Components/TransformComponent.h"

struct MovementComponent {
    std::vector<HexCoords> currentPath{};
    size_t currentPathIndex = 0;
    float stepTimer = 0.0f;
    float timePerStep = 0.15f;
    bool isMoving = false;
};

namespace MovementSystem {
    inline void CancelPath(MovementComponent *moveComp) {
        if (!moveComp) return;
        moveComp->isMoving = false;
        moveComp->currentPathIndex = 0;
        moveComp->stepTimer = 0.0f;
        moveComp->currentPath.clear();
    }

    inline void SetPath(Entity entity, std::vector<HexCoords> newPath) {
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

    inline void Update(Registry &registry, float dt, const HexGrid &grid) {
        registry.ForEach<MovementComponent, TransformComponent>(
            [&](Entity entity, MovementComponent *moveComp, TransformComponent *transComp) {
                if (!moveComp->isMoving) {
                    return;
                }

                if (moveComp->currentPathIndex >= moveComp->currentPath.size()) {
                    CancelPath(moveComp);
                    return;
                }

                moveComp->stepTimer += dt;
                if (moveComp->stepTimer >= moveComp->timePerStep) {
                    moveComp->stepTimer -= moveComp->timePerStep;
                    HexCoords targetHex = moveComp->currentPath[moveComp->currentPathIndex];
                    glm::vec2 targetWorldPos2D = grid.GetWorldPos(targetHex);

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
}

#endif //OBLIBERRY_MOVEMENT_H
