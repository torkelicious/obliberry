#ifndef OBLIBERRY_INTERACTIONSYSTEM_H
#define OBLIBERRY_INTERACTIONSYSTEM_H

#include "Core/EngineContext.h"
#include "Core/Window.h"
#include "Core/InputManager.h"
#include "Core/Constants.h"
#include "Renderer/Camera.h"
#include "Map/Hex.h"
#include "ECS/ECS.h"
#include "ECS/Components/PlayerInputComponent.h"
#include <algorithm>

namespace InteractionSystem {
    inline glm::vec2 Update(Registry &registry, const EngineContext &ctx, HexGrid &grid, HexCoords &outSelectedHex,
                            bool &outHasSelection) {
        float windowWidth = static_cast<float>(ctx.window->GetWidth());
        float windowHeight = static_cast<float>(ctx.window->GetHeight());

        // Camera zooming
        if (ctx.input->scrollY != 0.0) {
            ctx.camera->Zoom += static_cast<float>(ctx.input->scrollY) * ZOOM_SPEED;
            ctx.camera->Zoom = std::clamp(ctx.camera->Zoom, 0.5f, 5.0f);
        }

        constexpr float EDGE_MARGIN = 20.0f;
        glm::vec2 screenPan(0.0f);
        HexMath::Point mousePos = {
            static_cast<float>(ctx.input->mousePosX), static_cast<float>(ctx.input->mousePosY)
        };

        // Mouse Edge Panning
        if (mousePos.x <= EDGE_MARGIN) screenPan.x -= 1.0f;
        else if (mousePos.x >= windowWidth - EDGE_MARGIN) screenPan.x += 1.0f;
        if (mousePos.y <= EDGE_MARGIN) screenPan.y += 1.0f;
        else if (mousePos.y >= windowHeight - EDGE_MARGIN) screenPan.y -= 1.0f;

        registry.ForEach<PlayerInputComponent>([&](Entity entity, PlayerInputComponent *inputComp) {
            if (ctx.input->IsKeyDown(inputComp->Left)) screenPan.x -= 1.0f;
            if (ctx.input->IsKeyDown(inputComp->Right)) screenPan.x += 1.0f;
            if (ctx.input->IsKeyDown(inputComp->Up)) screenPan.y += 1.0f;
            if (ctx.input->IsKeyDown(inputComp->Down)) screenPan.y -= 1.0f;
            if (ctx.input->IsKeyPressed(inputComp->Quit)) {
                ctx.window->Close();
            }
        });

        if (glm::length(screenPan) > 0.0f) {
            screenPan = glm::normalize(screenPan);

            // Vertical Foreshortening Compensation
            constexpr float VERTICAL_COMPENSATION = 1.4f;
            screenPan.y *= VERTICAL_COMPENSATION;

            glm::mat4 invRot = glm::inverse(ctx.camera->GetRotation());
            glm::vec4 worldPan = invRot * glm::vec4(screenPan.x, screenPan.y, 0.0f, 0.0f);
            ctx.camera->Position += glm::vec2(worldPan.x, worldPan.y) * PAN_SPEED * ctx.deltaTime * (
                1.0f / ctx.camera->Zoom);
        }

        glm::vec2 worldPos = ctx.camera->MouseToWorld(mousePos.x, mousePos.y, windowWidth, windowHeight);

        // Global Hex Selection
        HexCoords hexPosOnMpos = HexMath::PixelToHex({worldPos.x, worldPos.y});
        if (grid.Get(hexPosOnMpos) == nullptr) {
            outHasSelection = false;
        } else {
            outSelectedHex = hexPosOnMpos;
            outHasSelection = true;
        }

        return worldPos;
    }
}

#endif //OBLIBERRY_INTERACTIONSYSTEM_H
