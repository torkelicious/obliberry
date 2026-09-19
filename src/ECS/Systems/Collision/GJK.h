#pragma once

#include "ColliderGeometry.h"
#include "glm/ext/quaternion_geometric.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/geometric.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>

namespace ECS::Collision {
    enum class GJKResult : uint8_t { Separated, Intersecting, Indeterminate };
    namespace Detail {
        struct Simplex {
            std::array<glm::dvec3, 4> points{};
            int count = 0;
        };

        inline bool IsFinite(const glm::dvec3 &v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }

        inline glm::dvec3 Reduce(Simplex &simplex) {
            const auto points = simplex.points;
            const int count = simplex.count;

            double bestDistSqred = std::numeric_limits<double>::infinity();

            glm::dvec3 closest(0.0);
            Simplex reduced;

            auto consider = [&](const glm::dvec3 &point, const std::initializer_list<int> indices) {
                const double distSqred = glm::dot(point, point);
                if (distSqred >= bestDistSqred)
                    return;

                bestDistSqred = distSqred;
                closest = point;
                reduced.count = 0;

                for (const int index : indices) {
                    reduced.points[reduced.count++] = points[index];
                }
            };

            // verts
            for (int i = 0; i < count; ++i) {
                consider(points[i], {i});
            }

            // segments
            for (int i = 0; i < count; ++i) {
                for (int j = i + 1; j < count; ++j) {
                    const glm::dvec3 edge = points[j] - points[i];
                    const double denominator = glm::dot(edge, edge);

                    if (denominator == 0.0) {
                        continue;
                    }

                    const double t = std::clamp(-glm::dot(points[i], edge) / denominator, 0.0, 1.0);
                    consider(points[i] + t * edge, {i, j});
                }
            }

            // inside triangle
            for (int i = 0; i < count; ++i) {
                for (int j = i + 1; j < count; ++j) {
                    for (int k = j + 1; k < count; ++k) {
                        const glm::dvec3 u = points[j] - points[i];
                        const glm::dvec3 v = points[k] - points[i];

                        const double uu = glm::dot(u, u);
                        const double uv = glm::dot(u, v);
                        const double vv = glm::dot(v, v);

                        const double denominator = uu * vv - uv * uv;

                        // skip basically inbred triangles
                        if (denominator <= 1e-14 * uu * vv) {
                            continue;
                        }

                        const double a = -glm::dot(points[i], u);
                        const double b = -glm::dot(points[i], v);

                        const double s = (a * vv - b * uv) / denominator;
                        const double t = (b * uu - a * uv) / denominator;

                        if (s >= 0.0 && t >= 0.0 && s + t <= 1.0) {
                            consider(points[i] + s * u + t * v, {i, j, k});
                        }
                    }
                }
            }

            // teeterahedon
            if (count == 4) {
                const glm::dmat3 edges(points[1] - points[0], points[2] - points[0], points[3] - points[0]);
                const double volume = glm::determinant(edges);
                const double scale = glm::length(edges[0]) * glm::length(edges[1]) * glm::length(edges[2]);
                if (std::abs(volume) > 1e-12 * scale) {
                    const glm::dvec3 weights = glm::inverse(edges) * (-points[0]);

                    if (weights.x >= 0.0 && weights.y >= 0.0 && weights.z >= 0.0 && weights.x + weights.y + weights.z <= 1.0) {
                        return glm::dvec3(0.0);
                    }
                }
            }
            simplex = reduced;
            return closest;
        }
    } // namespace Detail

    inline GJKResult IntersectsGJK(const WorldCollider &a, const WorldCollider &b, const double tolerance = CollisionTolerance) {
        if (!std::isfinite(tolerance) || tolerance <= 0.0)
            return GJKResult::Indeterminate;

        auto support = [&](const glm::dvec3 &direction) { return SupportWorld(a, direction) - SupportWorld(b, -direction); };

        glm::dvec3 direction = glm::dvec3(b.localToWorld[3] - a.localToWorld[3]);

        if (!Detail::IsFinite(direction))
            return GJKResult::Indeterminate;

        if (glm::dot(direction, direction) == 0.0)
            direction = {1.0, 0.0, 0.0};

        Detail::Simplex simplex;
        simplex.points[0] = support(glm::normalize(direction));
        simplex.count = 1;

        glm::dvec3 closest = simplex.points[0];

        for (int iteration = 0; iteration < 128; ++iteration) {
            const double distance = glm::length(closest);

            if (!std::isfinite(distance))
                return GJKResult::Indeterminate;

            // touching or within the tolerance
            if (distance <= tolerance)
                return GJKResult::Intersecting;

            direction = -closest / distance;

            const glm::dvec3 point = support(direction);

            if (!Detail::IsFinite(point))
                return GJKResult::Indeterminate;

            // separating plane proves the shapes are apart
            if (glm::dot(point, direction) < -tolerance)
                return GJKResult::Separated;

            // avoid cycling
            for (int i = 0; i < simplex.count; ++i) {
                if (glm::length(point - simplex.points[i]) <= tolerance * 1e-3) {
                    return GJKResult::Indeterminate;
                }
            }

            simplex.points[simplex.count++] = point;
            closest = Detail::Reduce(simplex);
        }

        return GJKResult::Indeterminate;
    }

} // namespace ECS::Collision
