#ifndef OBLIBERRY_RENDERSYSTEM_H
#define OBLIBERRY_RENDERSYSTEM_H

#include "ECS/ECS.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Renderer/Renderer.h"

namespace RenderSystem {
    inline void Render(Registry &registry, Renderer &renderer) noexcept {
        registry.ForEach<MeshComponent, MaterialComponent, TransformComponent>(
            [&](const Entity entity,
                const MeshComponent *meshComp,
                const MaterialComponent *matComp,
                const TransformComponent *transComp) {
                if (!meshComp || !meshComp->mesh) return;
                if (!matComp || !matComp->material) return;
                if (!transComp) return;

                const Texture *textureOverride = nullptr;

                if (const auto *dir = entity.GetComponent<DirectionalTextureComponent>()) {
                    if (!dir->textures.empty()) {
                        if (const auto idx = dir->index % dir->textures.size(); dir->textures[idx]) {
                            textureOverride = dir->textures[idx].get();
                        }
                    }
                }

                if (!matComp->material->shader) return;

                renderer.Submit(meshComp->mesh.get(), matComp->material.get(), transComp->transform, textureOverride);
            }
        );
    }
}

#endif //OBLIBERRY_RENDERSYSTEM_H
