#pragma once

#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/BillboardTagComponent.h"
#include "ECS/Registry.h"
#include "Rendering/Camera.h"

namespace ECS::Systems::SpriteBillboardSystem {
    [[nodiscard]] inline glm::mat4 MakeBillboardMatrix(const glm::vec3 &position, const float width, const float height, const glm::vec3 &right, const glm::vec3 &up) noexcept {
        const glm::vec3 forward = glm::cross(right, up);
        const glm::vec3 renderCenter = position + up * (height * 0.5f);
        auto model = glm::mat4(1.0f);
        model[0] = glm::vec4(right * width, 0.0f);
        model[1] = glm::vec4(up * height, 0.0f);
        model[2] = glm::vec4(forward, 0.0f);
        model[3] = glm::vec4(renderCenter, 1.0f);
        return model;
    }

    inline void Update(Registry &registry, const Rendering::Camera *camera, const Math::Frustum::FrustumPlanes &frustum3D) noexcept {
        if (!camera)
            return;
        const glm::vec3 right = camera->GetRightVector();
        const glm::vec3 up = camera->GetUpVector();

        registry.ForEach<Components::BillboardTagComponent, Components::TransformComponent>([&](Entity, Components::BillboardTagComponent *, Components::TransformComponent *transComp) {
            const glm::vec3 pos = transComp->transform.GetPosition();
            const glm::vec3 scale = transComp->transform.GetScale();
            if (const float radius = std::max(scale.x, scale.y) * 0.5f; !frustum3D.IntersectsSphere(pos, radius))
                return;
            transComp->transform.SetCustomMatrix(MakeBillboardMatrix(pos, scale.x, scale.y, right, up));
        });
    }
} // namespace ECS::Systems::SpriteBillboardSystem
