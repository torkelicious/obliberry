#pragma once

#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Math/Frustum.h"
#include "Rendering/Renderer.h"

namespace ECS::Systems::RenderSystem {
    inline void Render(Registry &registry, Rendering::Renderer &renderer, const Math::Frustum::FrustumPlanes &frustum3D) noexcept {
        auto *dirPool = registry.GetPool<Components::DirectionalTextureComponent>();

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

                    if (const auto *dir = dirPool->Get(static_cast<EntityID>(entity))) {
                        if (!dir->textures.empty()) {
                            if (const auto idx = dir->index % dir->textures.size(); dir->textures[idx]) {
                                renderer.Pin(dir->textures[idx]);
                                textureOverride = dir->textures[idx].get();
                            }
                        }
                    }

                    const int32_t entityInt = static_cast<int32_t>(static_cast<EntityID>(entity));
                    renderer.Submit(meshComp->mesh, matComp->material, transComp->worldTransform, textureOverride, entityInt);
                });
    }
} // namespace ECS::Systems::RenderSystem
