#ifndef OBLIBERRY_AISYSTEM_H
#define OBLIBERRY_AISYSTEM_H

#include "ECS/ECS.h"
#include "Map/Hex.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Systems/MovementSystem.h"
#include "ECS/Systems/DirectionalAnimationSystem.h"
#include <random>

namespace AISystem {
    inline void Update(Registry &registry, const float dt) noexcept {
        const MapComponent *map = nullptr;
        registry.ForEach<MapComponent>([&](Entity, const MapComponent *m) { map = m; });
        if (!map) return;

        HexCoords playerHex{0, 0};

        registry.ForEach<TransformComponent>(
            [&](const Entity entity, const TransformComponent *pt) {
                if (entity.GetName() == "Player") {
                    const glm::vec3 pos = pt->transform.GetPosition();
                    playerHex = Math::HexMath::PixelToHex({pos.x, pos.y});
                }
            }
        );

        thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<size_t> dist(0, map->grid.walkableTiles.size() - 1);
        std::uniform_real_distribution timeDist(1.0f, 3.0f);

        registry.ForEach<MovementComponent, TransformComponent, MaterialComponent>(
            [&](const Entity entity, MovementComponent *move, const TransformComponent *trans,
                const MaterialComponent *mat) {
                if (entity.GetName() == "Player") return;

                if (!move->isMoving) {
                    move->idleTimer -= dt;

                    if (move->idleTimer <= 0.0f) {
                        HexCoords target;
                        bool valid = false;
                        int attempts = 10;

                        while (!valid && attempts-- > 0) {
                            target = map->grid.walkableTiles[dist(rng)];
                            if (target != playerHex) valid = true;
                        }

                        if (valid) {
                            const glm::vec3 pos3 = trans->transform.GetPosition();
                            const HexCoords startHex = Math::HexMath::PixelToHex({pos3.x, pos3.y});

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
            }
        );
    }
}

#endif //OBLIBERRY_AISYSTEM_H
