#pragma once

#include "ECS/Registry.h"
#include "Math/GLMUtils.h"

namespace Editor::States::EntityGizmoDetail {
    inline bool ApplyWorldGizmo(Rendering::Transform &local, const glm::mat4 &before, const glm::mat4 &after, const glm::mat4 &parentWorld, const bool billboard) {
        if (before == after || !Math::GLMUtils::MatIsFinite(before) || !Math::GLMUtils::MatIsFinite(after) || !Math::GLMUtils::MatIsFinite(parentWorld) || std::abs(glm::determinant(parentWorld)) <= 1e-12f) {
            return false;
        }

        const glm::vec3 delta = glm::vec3(after[3] - before[3]);
        if (billboard || glm::mat3(before) == glm::mat3(after)) {
            const glm::vec3 position = local.GetPosition() + glm::inverse(glm::mat3(parentWorld)) * delta;
            glm::vec3 scale = local.GetScale();
            if (billboard) {
                const glm::vec3 oldWorldScale = Math::GLMUtils::MatrixScale(before);
                const glm::vec3 newWorldScale = Math::GLMUtils::MatrixScale(after);
                for (int axis = 0; axis < 3; ++axis) {
                    if (oldWorldScale[axis] == newWorldScale[axis])
                        continue;
                    if (oldWorldScale[axis] == 0.0f)
                        return false;
                    scale[axis] *= newWorldScale[axis] / oldWorldScale[axis];
                }
            }
            if (!Math::GLMUtils::VecIsFinite(position) || !Math::GLMUtils::VecIsFinite(scale) || (position == local.GetPosition() && scale == local.GetScale())) {
                return false;
            }
            local.SetPosition(position);
            local.SetScale(scale);
        } else {
            const glm::mat4 localMatrix = glm::inverse(parentWorld) * after;
            if (!Math::GLMUtils::MatIsFinite(localMatrix))
                return false;
            ECS::Registry::ApplyLocalFromWorld(local, localMatrix);
        }
        return true;
    }
} // namespace Editor::States::EntityGizmoDetail
