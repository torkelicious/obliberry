#ifndef OBLIBERRY_AISYSTEM_H
#define OBLIBERRY_AISYSTEM_H

#include "ECS/ECS.h"
#include "Map/Hex.h"
#include "ECS/Components/PlayerInputComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Systems/MovementSystem.h"
#include "ECS/Systems/DirectionalAnimationSystem.h"
#include <array>
#include <memory>
#include <random>

namespace AISystem {
    inline void Update(Registry &registry, float dt) {
        MapComponent *map = nullptr;
        registry.ForEach<MapComponent>([&](Entity, MapComponent *m) { map = m; });
        if (!map) return;

        HexCoords playerHex{0, 0};
        registry.ForEach<PlayerInputComponent, TransformComponent>(
            [&](Entity, PlayerInputComponent *, TransformComponent *pt) {
                glm::vec3 pos = pt->transform.GetPosition();
                playerHex = HexMath::PixelToHex({pos.x, pos.y});
            }
        );

        registry.ForEach<MovementComponent, TransformComponent, MaterialComponent>(
            [&](Entity entity, MovementComponent *move, TransformComponent *trans, MaterialComponent *mat) {
                if (entity.HasComponent<PlayerInputComponent>()) return;

                if (!move->isMoving && !map->grid.walkableTiles.empty()) {
                    move->idleTimer -= dt;
                    if (move->idleTimer <= 0.0f) {
                        HexCoords target;
                        bool valid = false;
                        int attempts = 10;

                        static thread_local std::mt19937 rng(std::random_device{}());
                        std::uniform_int_distribution<size_t> dist(0, map->grid.walkableTiles.size() - 1);

                        while (!valid && attempts-- > 0) {
                            target = map->grid.walkableTiles[dist(rng)];
                            if (target != playerHex) valid = true;
                        }

                        if (valid) {
                            glm::vec3 pos3 = trans->transform.GetPosition();
                            HexCoords startHex = HexMath::PixelToHex({pos3.x, pos3.y});
                            auto path = map->grid.FindPath(startHex, target);

                            if (!path.empty()) {
                                MovementSystem::SetPath(entity, std::move(path));
                            }

                            std::uniform_real_distribution<float> timeDist(1.0f, 3.0f);
                            move->idleTimer = timeDist(rng);
                        }
                    }
                }

                if (move->isMoving && move->currentPathIndex < move->currentPath.size()) {
                    const glm::vec3 pos3 = trans->transform.GetPosition();
                    const glm::vec2 pos{pos3.x, pos3.y};
                    const glm::vec2 target = HexMath::HexToWorld(move->currentPath[move->currentPathIndex]);
                    DirectionalAnimation::UpdateFacing(entity, target - pos, mat);
                }
            }
        );
    }
}

#endif //OBLIBERRY_AISYSTEM_H
