#pragma once


#include "ECS/ECS.h"
#include "Map/Hex.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Systems/MovementSystem.h"
#include "ECS/Systems/DirectionalAnimationSystem.h"
#include <random>

namespace ECS::Systems::AISystem {
    inline void Update(Registry &registry, const float dt) noexcept {
        const Components::MapComponent *map = nullptr;
        registry.ForEach<Components::MapComponent>([&](Entity, const Components::MapComponent *m) { map = m; });
        if (!map)
            return;

        Map::HexCoords playerHex{0, 0};

        registry.ForEach<Components::TransformComponent>(
                [&](const Entity entity, const Components::TransformComponent *pt) {
                    if (entity.GetName() == "Player") {
                        const glm::vec3 pos = pt->transform.GetPosition();
                        playerHex = Math::HexMath::PixelToHex({pos.x, pos.y});
                    }
                });

        if (map->grid.walkableTiles.empty())
            return;

        thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<size_t> dist(0, map->grid.walkableTiles.size() - 1);
        std::uniform_real_distribution timeDist(1.0f, 3.0f);

        registry.ForEach<Components::MovementComponent, Components::TransformComponent, Components::MaterialComponent>(
                [&](const Entity entity, Components::MovementComponent *move,
                    const Components::TransformComponent *trans, const Components::MaterialComponent *mat) {
                    if (entity.GetName() == "Player")
                        return;

                    if (!move->isMoving) {
                        move->idleTimer -= dt;

                        if (move->idleTimer <= 0.0f) {
                            Map::HexCoords target;
                            bool valid = false;
                            int attempts = 10;

                            while (!valid && attempts-- > 0) {
                                target = map->grid.walkableTiles[dist(rng)];
                                if (target != playerHex)
                                    valid = true;
                            }

                            if (valid) {
                                const glm::vec3 pos3 = trans->transform.GetPosition();
                                const Map::HexCoords startHex = Math::HexMath::PixelToHex({pos3.x, pos3.y});

                                map->grid.FindPath(startHex, target, move->currentPath);

                                if (!move->currentPath.empty()) {
                                    MovementSystem::StartPath(entity);
                                }

                                move->idleTimer = timeDist(rng);
                            }
                        }
                    }

                    if (move->isMoving && move->currentPathIndex < move->currentPath.size()) {
                        const glm::vec3 pos3 = trans->transform.GetPosition();
                        const glm::vec2 pos{pos3.x, pos3.y};
                        const glm::vec2 target = Math::HexMath::HexToWorld(move->currentPath[move->currentPathIndex]);
                        DirectionalAnimation::UpdateFacing(entity, target - pos, mat);
                    }
                });
    }
} // namespace ECS::Systems::AISystem
