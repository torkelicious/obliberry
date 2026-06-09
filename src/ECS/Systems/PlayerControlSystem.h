#ifndef OBLIBERRY_PLAYERCONTROLSYSTEM_H
#define OBLIBERRY_PLAYERCONTROLSYSTEM_H

#include "Core/EngineContext.h"
#include "Core/InputManager.h"
#include "Core/Constants.h"
#include "ECS/ECS.h"
#include "Map/Hex.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Systems/MovementSystem.h"
#include <array>
#include <memory>
#include "PlayerInputSystem.h"
#include "ECS/Components/PlayerInputComponent.h"

namespace PlayerControlSystem {
    inline void Update(Registry &registry, const EngineContext &ctx, HexGrid &grid, glm::vec2 worldPos,
                       float playerSpeed, const HexCoords &selectedHex, bool hasSelection, HexCoords &outPathTo,
                       bool &outHasPathTo, const std::array<std::shared_ptr<Texture>, 6> &textures) {
        registry.ForEach<PlayerInputComponent, TransformComponent, MovementComponent, MaterialComponent>(
            [&](Entity entity, PlayerInputComponent *input, TransformComponent *trans, MovementComponent *move,
                MaterialComponent *mat) {
                move->timePerStep = playerSpeed;
                glm::vec2 pPos = glm::vec2(trans->transform.GetPosition());

                glm::vec2 targetDir;
                bool hasTarget = false;

                if (move->isMoving && move->currentPathIndex < move->currentPath.size()) {
                    HexCoords nextHex = move->currentPath[move->currentPathIndex];
                    targetDir = HexMath::HexToWorld(nextHex) - pPos;
                    hasTarget = true;
                } else {
                    targetDir = worldPos - pPos;
                    if (glm::length(targetDir) > (HEX_SIZE - 0.005f)) hasTarget = true;
                }

                if (hasTarget) {
                    float degrees = glm::degrees(glm::atan(targetDir.y, targetDir.x));
                    if (degrees < 0.0f) degrees += 360.0f;
                    mat->material->texture = textures[static_cast<int>(std::lround(degrees / 60.0f)) % 6];
                }

                if (ctx.input->IsMousePressed(input->LeftClick) && hasSelection) {
                    HexCoords startHex = (move->isMoving && move->currentPathIndex < move->currentPath.size())
                                             ? move->currentPath[move->currentPathIndex]
                                             : HexMath::PixelToHex({pPos.x, pPos.y});

                    std::vector<HexCoords> path = grid.FindPath(startHex, selectedHex);
                    if (!path.empty()) {
                        outPathTo = selectedHex;
                        outHasPathTo = true;
                        MovementSystem::SetPath(entity, std::move(path));
                    }
                }

                if (!move->isMoving && outHasPathTo) outHasPathTo = false;
            }
        );
    }
}

#endif //OBLIBERRY_PLAYERCONTROLSYSTEM_H
