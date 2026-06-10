#ifndef OBLIBERRY_PLAYERCONTROLSYSTEM_H
#define OBLIBERRY_PLAYERCONTROLSYSTEM_H

#include "Core/EngineContext.h"
#include "ECS/ECS.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/PlayerInputComponent.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Systems/MovementSystem.h"
#include "ECS/Systems/DirectionalAnimationSystem.h"

namespace PlayerControlSystem {
    inline void Update(Registry &registry, const EngineContext &ctx, glm::vec2 worldPos) {
        MapComponent *map = nullptr;
        MapStateComponent *state = nullptr;

        registry.ForEach<MapComponent, MapStateComponent>([&](Entity, MapComponent *m, MapStateComponent *s) {
            map = m;
            state = s;
        });

        if (!map || !state) return;

        registry.ForEach<PlayerInputComponent, TransformComponent, MovementComponent, MaterialComponent>(
            [&](Entity entity, PlayerInputComponent *input, TransformComponent *trans, MovementComponent *move,
                MaterialComponent *mat) {
                glm::vec2 pPos = trans->transform.GetPosition();
                glm::vec2 targetDir;

                if (move->isMoving && move->currentPathIndex < move->currentPath.size()) {
                    HexCoords nextHex = move->currentPath[move->currentPathIndex];
                    targetDir = HexMath::HexToWorld(nextHex) - pPos;
                } else {
                    targetDir = worldPos - pPos;
                }

                DirectionalAnimation::UpdateFacing(entity, targetDir, mat);


                if (ctx.input->IsMousePressed(input->LeftClick) && state->hasSelection) {
                    HexCoords startHex = (move->isMoving && move->currentPathIndex < move->currentPath.size())
                                             ? move->currentPath[move->currentPathIndex]
                                             : HexMath::PixelToHex({pPos.x, pPos.y});

                    auto path = map->grid.FindPath(startHex, state->selectedHex);

                    if (!path.empty()) {
                        state->pathTo = state->selectedHex;
                        state->hasPathTo = true;
                        MovementSystem::SetPath(entity, std::move(path));
                    }
                }

                if (!move->isMoving && state->hasPathTo)
                    state->hasPathTo = false;
            }
        );
    }
}

#endif //OBLIBERRY_PLAYERCONTROLSYSTEM_H
