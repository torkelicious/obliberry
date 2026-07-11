#pragma once

#include <glm/glm.hpp>
#include "Rendering/Camera.h"
#include "Core/Constants.h"

namespace Math::Projection {
    struct AABB {
        glm::vec2 min{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        glm::vec2 max{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

        void Expand(const glm::vec2 &p) {
            min = glm::min(min, p);
            max = glm::max(max, p);
        }

        [[nodiscard]] bool Intersects(const AABB &other) const { return min.x <= other.max.x && max.x >= other.min.x && min.y <= other.max.y && max.y >= other.min.y; }
    };

    inline glm::vec2 UnprojectToGround(const glm::mat4 &invVP, const float ndcX, const float ndcY) {
        glm::vec4 nearWorld = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 farWorld = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
        nearWorld /= nearWorld.w;
        farWorld /= farWorld.w;

        const glm::vec3 rayDir = glm::vec3(farWorld) - glm::vec3(nearWorld);
        if (std::abs(rayDir.z) < 0.0001f)
            return {nearWorld.x, nearWorld.y};

        const float t = -nearWorld.z / rayDir.z;
        glm::vec3 hit = glm::vec3(nearWorld) + t * rayDir;
        return {hit.x, hit.y};
    }

    inline AABB GetCameraGroundAABB(const Rendering::Camera *camera, const float aspect) {
        AABB bounds;
        const glm::mat4 invVP = glm::inverse(camera->GetVP(aspect));

        // unproject the corners of the screen down to the z0 grid
        bounds.Expand(UnprojectToGround(invVP, -1.0f, -1.0f)); // Bottom-Left
        bounds.Expand(UnprojectToGround(invVP, 1.0f, -1.0f));  // Bottom-Right
        bounds.Expand(UnprojectToGround(invVP, 1.0f, 1.0f));   // Top-Right
        bounds.Expand(UnprojectToGround(invVP, -1.0f, 1.0f));  // Top-Left

        // padding buffer
        constexpr float padding = Core::HEX_SIZE * 2;
        bounds.min -= glm::vec2(padding);
        bounds.max += glm::vec2(padding);

        return bounds;
    }
} // namespace Math::Projection
