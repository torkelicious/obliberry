#ifndef OBLIBERRY_RENDERSYSTEM_H
#define OBLIBERRY_RENDERSYSTEM_H

#include "ECS/ECS.h"
#include "ECS/Components/DirectionalTextureComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Renderer/Renderer.h"

namespace RenderSystem {
    inline void Render(Registry &registry, Renderer &renderer) {
        registry.ForEach<MeshComponent, MaterialComponent, TransformComponent>(
            [&](Entity entity,
                MeshComponent *meshComp,
                MaterialComponent *matComp,
                TransformComponent *transComp) {
                if (!meshComp || !meshComp->mesh) return;
                if (!matComp || !matComp->material) return;
                if (!transComp) return;

                auto &mat = matComp->material;

                if (auto *dir = entity.GetComponent<DirectionalTextureComponent>()) {
                    if (!dir->textures.empty()) {
                        size_t idx = static_cast<size_t>(dir->index % dir->textures.size());

                        if (dir->textures[idx]) {
                            mat->texture = dir->textures[idx];
                        }
                    }
                }

                if (!mat->shader) return;

                renderer.Submit(*meshComp->mesh, *mat, transComp->transform);
            }
        );
    }
}

#endif //OBLIBERRY_RENDERSYSTEM_H
