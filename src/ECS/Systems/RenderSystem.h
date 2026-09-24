#pragma once

#include "ECS/Components/BillboardTagComponent.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/SpriteSheetComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Types.h"
#include "Math/Billboard.h"
#include "Math/Frustum.h"
#include "Rendering/Renderer.h"
#include "Rendering/Types/Camera.h"

namespace ECS::Systems::RenderSystem {
    inline void Render(Registry &registry, Rendering::Renderer &renderer, const Math::Frustum::FrustumPlanes &frustum3D, const Rendering::Camera *camera) noexcept {
        auto *dirPool = registry.GetPool<Components::DirectionalTextureComponent>();
        auto *animPool = registry.GetPool<Components::SpriteSheetComponent>();

        registry.ForEach<Components::MeshComponent, Components::MaterialComponent, Components::TransformComponent>(
                [&](const Entity entity, const Components::MeshComponent *meshComp, const Components::MaterialComponent *matComp, const Components::TransformComponent *transComp) {
            if (!meshComp || !meshComp->mesh)
                return;
            if (!matComp || !matComp->material)
                return;
            if (!transComp)
                return;

            const glm::vec3 &pos = transComp->worldTransform.GetPosition();
            const glm::vec3 &scale = transComp->worldTransform.GetScale();

            const float meshRadius = meshComp->mesh->GetBoundingRadius();
            const float maxScale = std::max({scale.x, scale.y, scale.z, 1.0f});

            if (const float worldRadius = meshRadius * maxScale; !frustum3D.IntersectsSphere(pos, worldRadius)) {
                return;
            }

            const Rendering::Texture *textureOverride = nullptr;
            glm::vec4 uvRect{0.0f, 0.0f, 1.0f, 1.0f};

            if (const auto *dir = dirPool->Get(static_cast<EntityID>(entity))) {
                if (const auto idx = dir->index % dir->textures.size(); dir->textures[idx]) {
                    renderer.Pin(dir->textures[idx]);
                    textureOverride = dir->textures[idx].get();
                }
            }

            if (const auto *sprite = animPool->Get(static_cast<EntityID>(entity)); sprite && sprite->sheet && sprite->sheet->texture) {
                renderer.Pin(sprite->sheet->texture);
                textureOverride = sprite->sheet->texture.get();
                uvRect = sprite->sheet->GetFrameUV(sprite->frame);
            }


            const auto entityInt = static_cast<int32_t>(static_cast<EntityID>(entity));
            Rendering::Transform renderTransform = transComp->worldTransform;
            if (camera && entity.HasComponent<Components::BillboardTagComponent>()) {
                const auto scale = transComp->worldTransform.GetScale();
                glm::mat4 billboard = Math::MakeBillboardMatrix(transComp->worldTransform.GetPosition(), scale.x, scale.y, camera->GetRightVector(), camera->GetUpVector());
                billboard[2] *= scale.z;
                renderTransform.SetCustomMatrix(billboard);
            }
            renderer.Submit(meshComp->mesh, matComp->material, renderTransform, textureOverride, entityInt, uvRect);
        });
    }
} // namespace ECS::Systems::RenderSystem
