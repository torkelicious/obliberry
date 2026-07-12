#pragma once


#include <vector>
#include <glm/glm.hpp>
#include "ECS/ECS.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MapComponent.h"

namespace ECS::Systems::MovementSystem {
    inline void CancelPath(Components::MovementComponent *moveComp) noexcept {
        if (!moveComp)
            return;
        moveComp->isMoving = false;
        moveComp->currentPathIndex = 0;
        moveComp->stepTimer = 0.0f;
        moveComp->currentPath.clear();
    }

    inline void StartPath(const Entity entity) noexcept {
        auto *moveComp = entity.GetComponent<Components::MovementComponent>();
        if (!moveComp)
            return;

        if (moveComp->currentPath.empty()) {
            CancelPath(moveComp);
            return;
        }

        moveComp->currentPathIndex = 0;
        moveComp->stepTimer = 0.0f;
        moveComp->isMoving = true;
    }

    inline void Update(Registry &registry, const float dt) noexcept {
        if (const Components::MapComponent *map = registry.GetFirst<Components::MapComponent>(); !map)
            return;

        registry.ForEach<Components::MovementComponent, Components::TransformComponent>([&](const Entity /*entity*/, Components::MovementComponent *moveComp, Components::TransformComponent *transComp) {
            if (!moveComp->isMoving)
                return;

            if (moveComp->currentPathIndex >= moveComp->currentPath.size()) {
                CancelPath(moveComp);
                return;
            }

            moveComp->stepTimer += dt;
            if (moveComp->stepTimer >= moveComp->timePerStep) {
                moveComp->stepTimer -= moveComp->timePerStep;
                const Map::HexCoords targetHex = moveComp->currentPath[moveComp->currentPathIndex];

                const glm::vec2 targetWorldPos2D = Map::HexGrid::GetWorldPos(targetHex);

                transComp->transform.SetPosition(glm::vec3(targetWorldPos2D.x, targetWorldPos2D.y, transComp->transform.GetPosition().z));

                moveComp->currentPathIndex++;
                if (moveComp->currentPathIndex >= moveComp->currentPath.size()) {
                    CancelPath(moveComp);
                }
            }
        });
    }

    inline void MoveToCenter(const Entity entity) noexcept {
        auto *moveComp = entity.GetComponent<Components::MovementComponent>();
        auto *transComp = entity.GetComponent<Components::TransformComponent>();

        if (!moveComp || !transComp)
            return;

        CancelPath(moveComp);

        transComp->transform.SetPosition(glm::vec3(0.0f, 0.0f, transComp->transform.GetPosition().z));
    }
} // namespace ECS::Systems::MovementSystem
