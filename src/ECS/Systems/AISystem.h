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
#include <array>
#include <memory>
#include <random>

namespace AISystem {
    inline void Update(Registry &registry) {
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

                move->timePerStep = 0.15f;

                if (!move->isMoving && !map->grid.tiles.empty()) {
                    HexCoords target;
                    bool valid = false;
                    int attempts = 10;

                    std::mt19937 rng(std::random_device{}());
                    std::uniform_int_distribution<size_t> dist(0, map->grid.tiles.size() - 1);

                    while (!valid && attempts-- > 0) {
                        auto it = std::ranges::next(map->grid.tiles.begin(), dist(rng));
                        target = it->first;
                        if (target != playerHex) valid = true;
                    }

                    if (valid) {
                        glm::vec3 pos3 = trans->transform.GetPosition();
                        HexCoords startHex = HexMath::PixelToHex({pos3.x, pos3.y});
                        auto path = map->grid.FindPath(startHex, target);

                        if (!path.empty()) {
                            MovementSystem::SetPath(entity, std::move(path));
                        }
                    }
                }

                if (move->isMoving && move->currentPathIndex < move->currentPath.size()) {
                    glm::vec3 pos3 = trans->transform.GetPosition();
                    glm::vec2 pos(pos3.x, pos3.y);
                    glm::vec2 targetPos = HexMath::HexToWorld(move->currentPath[move->currentPathIndex]);
                    glm::vec2 dir = targetPos - pos;

                    float len = glm::length(dir);
                    if (len > 0.001f) {
                        float degrees = glm::degrees(std::atan2(dir.y, dir.x));
                        if (degrees < 0.0f) degrees += 360.0f;
                        int index = static_cast<int>(std::lround(degrees / 60.0f)) % 6;

                        if (auto *dirComp = entity.GetComponent<DirectionalTextureComponent>()) {
                            dirComp->index = index;
                            if (mat->material && dirComp->textures[index]) {
                                mat->material->texture = dirComp->textures[index];
                            }
                        }
                    }
                }
            }
        );
    }
}

#endif //OBLIBERRY_AISYSTEM_H
