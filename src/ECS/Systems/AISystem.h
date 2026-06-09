#ifndef OBLIBERRY_AISYSTEM_H
#define OBLIBERRY_AISYSTEM_H

#include "ECS/ECS.h"
#include "Map/Hex.h"
#include "ECS/Components/PlayerInputComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Systems/MovementSystem.h"
#include <array>
#include <memory>
#include <cstdlib>

// basic wandeering lmao
namespace AISystem {
    inline void Update(Registry &registry, HexGrid &grid, float playerSpeed,
                       const std::array<std::shared_ptr<Texture>, 6> &textures) {
        registry.ForEach<MovementComponent, TransformComponent, MaterialComponent>(
            [&](Entity entity, MovementComponent *move, TransformComponent *trans, MaterialComponent *mat) {
                if (entity.HasComponent<PlayerInputComponent>()) return;

                move->timePerStep = playerSpeed;

                if (!move->isMoving && !grid.tiles.empty()) {
                    HexCoords target;
                    bool validTargetFound = false;
                    int maxAttempts = 10;

                    HexCoords playerHex{0, 0};
                    registry.ForEach<PlayerInputComponent, TransformComponent>(
                        [&](Entity e, PlayerInputComponent *p, TransformComponent *pt) {
                            playerHex = HexMath::PixelToHex({
                                pt->transform.GetPosition().x, pt->transform.GetPosition().y
                            });
                        });

                    while (!validTargetFound && maxAttempts-- > 0) {
                        auto it = std::ranges::next(grid.tiles.begin(), std::rand() % grid.tiles.size());
                        target = it->first;
                        if (target != playerHex) validTargetFound = true;
                    }

                    if (validTargetFound) {
                        glm::vec3 npcPos = trans->transform.GetPosition();
                        std::vector<HexCoords> npcPath = grid.FindPath(HexMath::PixelToHex({npcPos.x, npcPos.y}),
                                                                       target);
                        if (!npcPath.empty()) MovementSystem::SetPath(entity, npcPath);
                    }
                }

                if (move->isMoving && move->currentPathIndex < move->currentPath.size()) {
                    glm::vec2 nPos = glm::vec2(trans->transform.GetPosition());
                    glm::vec2 targetDirW = HexMath::HexToWorld(move->currentPath[move->currentPathIndex]) - nPos;

                    if (glm::length(targetDirW) > 0.01f) {
                        float degrees = glm::degrees(glm::atan(targetDirW.y, targetDirW.x));
                        if (degrees < 0.0f) degrees += 360.0f;
                        mat->material->texture = textures[static_cast<int>(std::lround(degrees / 60.0f)) % 6];
                    }
                }
            }
        );
    }
}

#endif //OBLIBERRY_AISYSTEM_H
