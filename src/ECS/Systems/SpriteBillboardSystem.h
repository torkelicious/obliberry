#ifndef OBLIBERRY_SPRITEBILLBOARDSYSTEM_H
#define OBLIBERRY_SPRITEBILLBOARDSYSTEM_H
#include "../Components/TransformComponent.h"
#include "ECS/Components/BillboardComponent.h"
#include "ECS/Registry.h"
#include "Renderer/Camera.h"

namespace SpriteBillboardSystem {
    inline glm::mat4 GetBillboardMatrix(const glm::vec3 &position, float width, float height, const Camera &camera) {
        glm::vec3 right = camera.GetRightVector();
        glm::vec3 up = camera.GetUpVector();
        glm::vec3 forward = glm::cross(right, up);
        glm::vec3 renderCenter = position + up * (height * 0.5f);
        auto model = glm::mat4(1.0f);
        model[0] = glm::vec4(right * width, 0.0f);
        model[1] = glm::vec4(up * height, 0.0f);
        model[2] = glm::vec4(forward, 0.0f);
        model[3] = glm::vec4(renderCenter, 1.0f);
        return model;
    }

    inline void Update(Registry &registry, const Camera *camera) {
        if (!camera) return;

        registry.ForEach<BillboardComponent, TransformComponent>(
            [&](Entity, BillboardComponent *, TransformComponent *transComp) {
                const glm::vec3 pos = transComp->transform.GetPosition();
                const glm::vec3 scale = transComp->transform.GetScale();
                transComp->transform.SetCustomMatrix(GetBillboardMatrix(pos, scale.x, scale.y, *camera));
            });
    }
}

#endif //OBLIBERRY_SPRITEBILLBOARDSYSTEM_H
