#ifndef OBLIBERRY_INTERACTIONSYSTEM_H
#define OBLIBERRY_INTERACTIONSYSTEM_H

#include "Core/EngineContext.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/PlayerInputComponent.h"
#include "ECS/ECS.h"
#include "Map/Hex.h"
#include <algorithm>

#include "Core/Window.h"
#include "Renderer/Camera.h"

namespace InteractionSystem {
    [[nodiscard]] inline glm::vec2 Update(Registry &registry, const EngineContext &ctx) noexcept {
        const float windowWidth = static_cast<float>(ctx.window->GetWidth());
        const float windowHeight = static_cast<float>(ctx.window->GetHeight());

        if (ctx.input->ScrollY() != 0.0) {
            ctx.camera->Zoom += static_cast<float>(ctx.input->ScrollY()) * ZOOM_SPEED;
            ctx.camera->Zoom = std::clamp(ctx.camera->Zoom, 0.5f, 5.0f);
        }

        constexpr float EDGE_MARGIN = 20.0f;
        glm::vec2 screenPan(0.0f);
        const glm::vec2 mousePos{
            static_cast<float>(ctx.input->MousePosX()),
            static_cast<float>(ctx.input->MousePosY())
        };

        if (mousePos.x <= EDGE_MARGIN)
            screenPan.x -= 1.0f;
        else if (mousePos.x >= windowWidth - EDGE_MARGIN)
            screenPan.x += 1.0f;
        if (mousePos.y <= EDGE_MARGIN)
            screenPan.y += 1.0f;
        else if (mousePos.y >= windowHeight - EDGE_MARGIN)
            screenPan.y -= 1.0f;

        registry.ForEach<PlayerInputComponent>(
            [&](Entity entity, const PlayerInputComponent *inputComp) {
                if (ctx.input->IsKeyDown(inputComp->Left))
                    screenPan.x -= 1.0f;
                if (ctx.input->IsKeyDown(inputComp->Right))
                    screenPan.x += 1.0f;
                if (ctx.input->IsKeyDown(inputComp->Up))
                    screenPan.y += 1.0f;
                if (ctx.input->IsKeyDown(inputComp->Down))
                    screenPan.y -= 1.0f;
                if (ctx.input->IsKeyPressed(inputComp->Quit))
                    ctx.window->Close();
            });

        if (glm::length(screenPan) > 0.0f) {
            screenPan = glm::normalize(screenPan);
            constexpr float VERTICAL_COMPENSATION = 1.4f;
            screenPan.y *= VERTICAL_COMPENSATION;
            const glm::mat4 invRot = glm::inverse(ctx.camera->GetRotation());
            const glm::vec4 worldPan =
                    invRot * glm::vec4(screenPan.x, screenPan.y, 0.0f, 0.0f);
            ctx.camera->Position += glm::vec3(worldPan.x, worldPan.y, 0.0f) * PAN_SPEED *
                    ctx.deltaTime * (1.0f / ctx.camera->Zoom);
        }

        glm::vec2 worldPos = ctx.camera->MouseToWorld(mousePos.x, mousePos.y,
                                                      windowWidth, windowHeight);
        const HexCoords hexPosOnMpos = Math::HexMath::PixelToHex({worldPos.x, worldPos.y});

        registry.ForEach<MapComponent, MapStateComponent>(
            [&](Entity, MapComponent *mapComp, MapStateComponent *stateComp) {
                const auto *tile = mapComp->grid.Get(hexPosOnMpos);
                if (tile == nullptr) {
                    stateComp->hasSelection = false;
                } else {
                    stateComp->selectedHex = hexPosOnMpos;
                    stateComp->hasSelection = true;
                }
            });

        return worldPos;
    }
}

#endif // OBLIBERRY_INTERACTIONSYSTEM_H
