#pragma once

#include "ECS/ECS.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Systems/DirectionalAnimationSystem.h"
#include "Math/HexMath.h"
#include "Core/EngineContext.h"
#include "Core/InputManager.h"
#include "Core/Window.h"
#include "Renderer/Camera.h"

namespace PlayerControlSystem {
    inline void Update(Registry &registry, const EngineContext &ctx) noexcept {
        glm::vec2 worldPos{0.0f, 0.0f};
        if (ctx.camera && ctx.window && ctx.input) {
            worldPos = ctx.camera->MouseToWorld(
                static_cast<float>(ctx.input->MousePosX()),
                static_cast<float>(ctx.input->MousePosY()),
                static_cast<float>(ctx.window->GetWidth()),
                static_cast<float>(ctx.window->GetHeight())
            );
        }
        registry.ForEach<TransformComponent, MovementComponent, MaterialComponent>(
            [&](const Entity entity, const TransformComponent *trans, const MovementComponent *move,
                const MaterialComponent *mat) {
                const glm::vec2 pPos = trans->transform.GetPosition();
                glm::vec2 targetDir;
                if (move->isMoving && move->currentPathIndex < move->currentPath.size()) {
                    const HexCoords nextHex = move->currentPath[move->currentPathIndex];
                    targetDir = Math::HexMath::HexToWorld(nextHex) - pPos;
                } else {
                    targetDir = worldPos - pPos;
                }

                DirectionalAnimation::UpdateFacing(entity, targetDir, mat);
            }
        );
    }
}
