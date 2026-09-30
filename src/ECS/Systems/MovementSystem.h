#pragma once


#include <vector>
#include <glm/glm.hpp>
#include "ECS/Components/RelationshipComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Systems/Collision/ColliderGeometry.h"
#include "ECS/Systems/Collision/CollisionQueries.h"
#include "ECS/Types.h"
#include "Map/Hex.h"
#include "glm/geometric.hpp"

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

    inline void Update(Registry &registry, const float dt, const Collision::BillboardBasis &basis) noexcept {
        const auto *map = registry.GetFirst<Components::MapComponent>();
        if (!map) {
            return;
        }

        registry.ForEach<Components::MovementComponent, Components::TransformComponent>([&](const Entity entity, Components::MovementComponent *moveComp, const Components::TransformComponent *transComp) {
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

                const Map::Tile *tile = map->grid.Get(targetHex);
                if (!tile || !tile->walkable) {
                    CancelPath(moveComp);
                    return;
                }

                const EntityID entID = static_cast<EntityID>(entity);

                const auto *relationship = registry.GetComponent<Components::RelationshipComponent>(entID);
                if (relationship && relationship->parent != INVALID_ENTITY_ID) {
                    CancelPath(moveComp); // should be unparented!!!
                    return;
                }

                const glm::vec2 targetXY = Map::HexGrid::GetWorldPos(targetHex);
                const glm::vec3 currPos = transComp->transform.GetPosition();
                const glm::vec3 targetPos{targetXY.x, targetXY.y, currPos.z};

                // would be no op
                if (glm::dot(targetPos - currPos, targetPos - currPos) > 1e-12f) {
                    if (!Collision::TryMoveTo(registry, entID, targetPos, basis)) {
                        CancelPath(moveComp);
                        return;
                    }
                }

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
