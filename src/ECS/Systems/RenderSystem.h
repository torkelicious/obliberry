#pragma once


#include "ECS/ECS.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Math/Frustum.h"
#include "Renderer/Renderer.h"

namespace RenderSystem {
    inline void Render(Registry &registry, Renderer &renderer,
                       const Math::Frustum::FrustumPlanes &frustum3D) noexcept {
        registry.ForEach<MeshComponent, MaterialComponent, TransformComponent>(
            [&](const Entity entity,
                const MeshComponent *meshComp,
                const MaterialComponent *matComp,
                const TransformComponent *transComp) {
                if (!meshComp || !meshComp->mesh) return;
                if (!matComp || !matComp->material) return;
                if (!transComp) return;

                const glm::vec3 &pos = transComp->transform.GetPosition();
                const glm::vec3 &scale = transComp->transform.GetScale();

                const float meshRadius = meshComp->mesh->GetBoundingRadius();
                const float maxScale = std::max({scale.x, scale.y, scale.z, 1.0f});

                if (const float worldRadius = meshRadius * maxScale; !frustum3D.IntersectsSphere(pos, worldRadius)) {
                    return;
                }

                const Texture *textureOverride = nullptr;

                if (const auto *dir = entity.GetComponent<DirectionalTextureComponent>()) {
                    if (!dir->textures.empty()) {
                        if (const auto idx = dir->index % dir->textures.size(); dir->textures[idx]) {
                            textureOverride = dir->textures[idx].get();
                        }
                    }
                }

                if (!matComp->material->shader) return;

                renderer.Submit(meshComp->mesh, matComp->material.get(), transComp->transform, textureOverride);
            }
        );
    }
}
