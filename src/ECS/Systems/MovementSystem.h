#pragma once


#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>
#include <glm/glm.hpp>
#include "ECS/Components/RelationshipComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Registry.h"
#include "ECS/Systems/Collision/ColliderGeometry.h"
#include "ECS/Systems/Collision/CollisionQueries.h"
#include "ECS/Types.h"
#include "Map/Hex.h"
#include "Map/HexCoords.h"
#include "Math/HexMath.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"

namespace ECS::Systems::MovementSystem {
    inline void CancelPath(Components::MovementComponent *moveComp) noexcept {
        if (!moveComp)
            return;
        moveComp->isMoving = false;
        moveComp->currentPathIndex = 0;
        moveComp->stepTimer = 0.0f;
        moveComp->replanAttempts = 0;
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
        moveComp->replanAttempts = 0;
        moveComp->isMoving = true;
    }


    inline void CollectOccupiedHexes(Registry &registry, std::unordered_set<Map::HexCoords, Map::HexCoordsHash> &out, const Collision::BillboardBasis &basis, const EntityID ignore = INVALID_ENTITY_ID) noexcept {
        out.clear();

        static constexpr float CLEARANCE = 0.5f * Core::SQRT_3 * Core::HEX_SIZE;

        registry.ForEach<Components::ColliderComponent, Components::TransformComponent>([&](const Entity otherEnt, const Components::ColliderComponent *col, const Components::TransformComponent *trans) {
            const EntityID otherId = static_cast<EntityID>(otherEnt);
            if (otherId == ignore)
                return;
            if (col->isTrigger || !Components::IsValidCollider(*col))
                return;

            const Collision::WorldCollider blocker = Collision::BuildWorldCollider(otherId, *col, *trans, basis);
            const Collision::ColliderAABB bounds = Collision::ComputeWorldAABB(blocker);
            const glm::dvec3 blockerCenter = (bounds.min + bounds.max) * 0.5;
            const Map::HexCoords center = Math::HexMath::PixelToHex({static_cast<float>(blockerCenter.x), static_cast<float>(blockerCenter.y)});
            out.insert(center);

            const double halfExtent = std::max(bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y) * 0.5;
            const int rings = std::clamp(static_cast<int>(std::ceil(halfExtent + CLEARANCE) / CLEARANCE), 2, 40);

            std::vector<Map::HexCoords> ring{center};
            std::unordered_set<Map::HexCoords, Map::HexCoordsHash> visited{center};

            for (int i = 0; i < rings; ++i) {
                std::vector<Map::HexCoords> next;
                for (const auto &hex : ring) {
                    for (const auto &neighbor : Math::HexMath::GetNeighbors(hex)) {
                        if (!visited.insert(neighbor).second)
                            continue;
                        next.push_back(neighbor);

                        const glm::vec2 world = Map::HexGrid::GetWorldPos(neighbor);
                        if (world.x > bounds.min.x - CLEARANCE && world.x < bounds.max.x + CLEARANCE && world.y > bounds.min.y - CLEARANCE && world.y < bounds.max.y + CLEARANCE)
                            out.insert(neighbor);
                    }
                }
                ring = std::move(next);
            }
        });
    }


    inline void ReplanAroundBlocker(
            Registry &registry, const Components::MapComponent &map, const Entity entity, Components::MovementComponent *moveComp, const Map::HexCoords blockedHex, const Collision::BillboardBasis &basis) noexcept {

        constexpr uint32_t MAX_REPLANS = 3;

        if (++moveComp->replanAttempts > MAX_REPLANS) {
            CancelPath(moveComp);
            return;
        }

        auto *transComp = entity.GetComponent<Components::TransformComponent>();
        if (!transComp) {
            CancelPath(moveComp);
            return;
        }

        const glm::vec3 pos = transComp->transform.GetPosition();
        const Map::HexCoords current = Math::HexMath::PixelToHex({pos.x, pos.y});
        const Map::HexCoords goal = moveComp->currentPath.back();

        std::unordered_set<Map::HexCoords, Map::HexCoordsHash> blocked;
        CollectOccupiedHexes(registry, blocked, basis, static_cast<EntityID>(entity));
        blocked.insert(blockedHex);

        std::vector<Map::HexCoords> newPath;
        map.grid.FindPath(current, goal, newPath, &blocked);
        if (newPath.empty()) {
            CancelPath(moveComp);
            return;
        }

        const size_t first = (newPath.size() > 1 && newPath.front() == current) ? 1U : 0U;
        moveComp->currentPath.assign(newPath.begin() + static_cast<std::ptrdiff_t>(first), newPath.end());
        moveComp->currentPathIndex = 0;
        moveComp->stepTimer = 0.0f;
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
                        ReplanAroundBlocker(registry, *map, entity, moveComp, targetHex, basis);
                        return;
                    }
                    moveComp->replanAttempts = 0;
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
