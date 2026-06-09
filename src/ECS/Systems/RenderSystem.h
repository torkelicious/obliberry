#ifndef OBLIBERRY_RENDERSYSTEM_H
#define OBLIBERRY_RENDERSYSTEM_H
#include <span>
#include "ECS/ECS.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Renderer/Renderer.h"

namespace RenderSystem {
    inline void Render(Registry &registry, Renderer &renderer) {
        registry.ForEach<MeshComponent, MaterialComponent, TransformComponent>(
            [&](Entity entity, MeshComponent *meshComp, MaterialComponent *matComp, TransformComponent *transComp) {
                if (meshComp->mesh) {
                    renderer.Submit(*(meshComp->mesh), *matComp->material, transComp->transform);
                }
            }
        );
    }
}

#endif //OBLIBERRY_RENDERSYSTEM_H
