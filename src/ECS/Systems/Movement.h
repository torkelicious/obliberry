#ifndef OBLIBERRY_MOVEMENT_H
#define OBLIBERRY_MOVEMENT_H

#include <vector>
#include <glm/glm.hpp>
#include "ECS/ECS.h"
#include "Map/Hex.h"
#include "ECS/Components/TransformComponent.h"

struct MovementComponent : public Component {
    std::vector<HexCoords> currentPath;
    int currentPathIndex = 0;

    float stepTimer = 0.0f;
    float timePerStep = 0.15f;

    bool isMoving = false;

    void Cancel() {
        isMoving = false;
        currentPathIndex = 0;
        stepTimer = 0.0f;
        currentPath.clear();
    }
};

namespace MovementSystem {
    inline void Update(float dt, Entity &entity, const HexGrid &grid) {
        auto *moveComp = entity.GetComponent<MovementComponent>();
        auto *transComp = entity.GetComponent<TransformComponent>();

        if (!moveComp || !transComp || !moveComp->isMoving) {
            return;
        }

        if (moveComp->currentPathIndex >= moveComp->currentPath.size()) {
            moveComp->Cancel();
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
                moveComp->Cancel();
            }
        }
    }

    inline void SetPath(const Entity &entity, const std::vector<HexCoords> &newPath) {
        auto *moveComp = entity.GetComponent<MovementComponent>();
        if (!moveComp) return;
        if (newPath.empty()) {
            moveComp->Cancel();
            return;
        }
        moveComp->currentPath = newPath;
        moveComp->currentPathIndex = 0;
        moveComp->stepTimer = 0.0f;
        moveComp->isMoving = true;
    }
}

#endif //OBLIBERRY_MOVEMENT_H
