#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <algorithm>
#include <array>
#include <limits>
#include "Renderer/Camera.h"
#include "Core/Constants.h"

namespace Math::Frustum {
    struct FrustumPlanes {
        std::array<glm::vec4, 6> planes{};


        static FrustumPlanes FromVP(const glm::mat4 &vp) noexcept {
            FrustumPlanes fp;

            const glm::vec4 row3(vp[0].w, vp[1].w, vp[2].w, vp[3].w);
            const glm::vec4 row0(vp[0].x, vp[1].x, vp[2].x, vp[3].x);
            const glm::vec4 row1(vp[0].y, vp[1].y, vp[2].y, vp[3].y);
            const glm::vec4 row2(vp[0].z, vp[1].z, vp[2].z, vp[3].z);

            fp.planes[0] = row3 + row0;
            fp.planes[1] = row3 - row0;
            fp.planes[2] = row3 + row1;
            fp.planes[3] = row3 - row1;
            fp.planes[4] = row3 + row2;
            fp.planes[5] = row3 - row2;

            for (auto &p: fp.planes) {
                if (const float len = glm::length(glm::vec3(p)); len > 1e-8f) {
                    p /= len;
                }
            }
            return fp;
        }

        [[nodiscard]] static float DistanceTo(const glm::vec4 &plane, const glm::vec3 &point) noexcept {
            return plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w;
        }

        [[nodiscard]] bool IntersectsSphere(const glm::vec3 &centre, const float radius) const noexcept {
            for (const auto &plane: planes) {
                if (DistanceTo(plane, centre) < -radius)
                    return false;
            }
            return true;
        }

        [[nodiscard]] bool IntersectsAABB(const glm::vec3 &min, const glm::vec3 &max) const noexcept {
            for (const auto &plane: planes) {
                const glm::vec3 p = {
                    plane.x >= 0.0f ? min.x : max.x,
                    plane.y >= 0.0f ? min.y : max.y,
                    plane.z >= 0.0f ? min.z : max.z,
                };
                if (DistanceTo(plane, p) < 0.0f)
                    return false;
            }
            return true;
        }
    };

    struct ViewFrustum {
        glm::vec2 minBounds{std::numeric_limits<float>::max()};
        glm::vec2 maxBounds{std::numeric_limits<float>::lowest()};

        [[nodiscard]] bool IsVisible(const glm::vec3 position, const float radius = 0.0f) const noexcept {
            return position.x + radius >= minBounds.x &&
                   position.x - radius <= maxBounds.x &&
                   position.y + radius >= minBounds.y &&
                   position.y - radius <= maxBounds.y;
        }

        [[nodiscard]] bool IsVisible(const glm::vec2 position, const float radius = 0.0f) const noexcept {
            return position.x + radius >= minBounds.x &&
                   position.x - radius <= maxBounds.x &&
                   position.y + radius >= minBounds.y &&
                   position.y - radius <= maxBounds.y;
        }

        [[nodiscard]] bool Contains(const glm::vec2 point) const noexcept {
            return point.x >= minBounds.x && point.x <= maxBounds.x &&
                   point.y >= minBounds.y && point.y <= maxBounds.y;
        }

        [[nodiscard]] bool Contains(const glm::vec3 point) const noexcept {
            return point.x >= minBounds.x && point.x <= maxBounds.x &&
                   point.y >= minBounds.y && point.y <= maxBounds.y;
        }
    };

    inline ViewFrustum FromCamera(const Camera *camera, const float aspect, const float padding = 0.0f) {
        ViewFrustum frustum;
        if (!camera) return frustum;

        const glm::mat4 invVP = glm::inverse(camera->GetVP(aspect));

        auto projectToGround = [&](const float ndcX, const float ndcY) -> glm::vec2 {
            glm::vec4 nearW = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
            glm::vec4 farW = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
            nearW /= nearW.w;
            farW /= farW.w;

            const glm::vec3 dir = glm::vec3(farW) - glm::vec3(nearW);
            if (std::abs(dir.z) < 1e-4f) return {nearW.x, nearW.y};

            const float t = -nearW.z / dir.z;
            glm::vec3 hit = glm::vec3(nearW) + t * dir;
            return {hit.x, hit.y};
        };

        auto expand = [&](const float nx, const float ny) {
            const glm::vec2 p = projectToGround(nx, ny);
            frustum.minBounds = glm::min(frustum.minBounds, p);
            frustum.maxBounds = glm::max(frustum.maxBounds, p);
        };

        expand(-1.0f, -1.0f); // bottom-left
        expand(1.0f, -1.0f); // bottom-right
        expand(1.0f, 1.0f); // top-right
        expand(-1.0f, 1.0f); // top-left

        frustum.minBounds -= glm::vec2(padding);
        frustum.maxBounds += glm::vec2(padding);

        return frustum;
    }

    inline ViewFrustum FromCameraVP(const glm::mat4 &vp, const float padding = 0.0f) noexcept {
        ViewFrustum frustum;
        const glm::mat4 invVP = glm::inverse(vp);

        auto projectToGround = [&](const float ndcX, const float ndcY) -> glm::vec2 {
            glm::vec4 nearW = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
            glm::vec4 farW = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
            nearW /= nearW.w;
            farW /= farW.w;

            const glm::vec3 dir = glm::vec3(farW) - glm::vec3(nearW);
            if (std::abs(dir.z) < 1e-4f) return {nearW.x, nearW.y};

            const float t = -nearW.z / dir.z;
            glm::vec3 hit = glm::vec3(nearW) + t * dir;
            return {hit.x, hit.y};
        };

        auto expand = [&](const float nx, const float ny) {
            const glm::vec2 p = projectToGround(nx, ny);
            frustum.minBounds = glm::min(frustum.minBounds, p);
            frustum.maxBounds = glm::max(frustum.maxBounds, p);
        };

        expand(-1.0f, -1.0f);
        expand(1.0f, -1.0f);
        expand(1.0f, 1.0f);
        expand(-1.0f, 1.0f);

        frustum.minBounds -= glm::vec2(padding);
        frustum.maxBounds += glm::vec2(padding);
        return frustum;
    }

    inline FrustumPlanes FromCamera3D(const Camera *camera, const float aspect) noexcept {
        if (!camera) return FrustumPlanes{};
        return FrustumPlanes::FromVP(camera->GetVP(aspect));
    }
}
