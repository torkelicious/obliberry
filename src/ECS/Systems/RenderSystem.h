#ifndef OBLIBERRY_RENDERSYSTEM_H
#define OBLIBERRY_RENDERSYSTEM_H
#include <span>
#include "ECS/ECS.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Renderer/Renderer.h"

namespace RenderSystem {
    inline void Render(std::span<const Entity> entities, Renderer &renderer) {
        for (const Entity entity: entities) {
            auto *meshComp = entity.GetComponent<MeshComponent>();
            auto *matComp = entity.GetComponent<MaterialComponent>();
            auto *transComp = entity.GetComponent<TransformComponent>();

            if (meshComp && meshComp->mesh && matComp && transComp) {
                renderer.Submit(*(meshComp->mesh), matComp->material, transComp->transform);
            }
        }
    }
}


#endif //OBLIBERRY_RENDERSYSTEM_H
