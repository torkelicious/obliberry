#pragma once

#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Types.h"
#include "Math/Billboard.h"
#include "glm/geometric.hpp"
#include "glm/matrix.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace ECS::Collision {

    inline constexpr double CollisionTolerance = 1e-6;

    struct BillboardBasis {
        glm::vec3 right{1.0f, 0.0f, 0.0f};
        glm::vec3 up{0.0f, 1.0f, 0.0f};
    };

    struct WorldCollider {
        ECS::EntityID entity = INVALID_ENTITY_ID;
        Components::ColliderComponent geometry;
        glm::dmat4 localToWorld{1.0f};
    };

    inline WorldCollider BuildWorldCollider(const EntityID entity, const Components::ColliderComponent &collider, const Components::TransformComponent &transform, const BillboardBasis &basis) {
        glm::mat4 frame = transform.worldTransform.GetMatrix();
        if (collider.orientation == Components::ColliderOrientation::Billboard) {
            const auto &position = transform.worldTransform.GetPosition();
            const auto &scale = transform.worldTransform.GetScale();

            frame = Math::MakeBillboardMatrix(position, scale.x, scale.y, basis.right, basis.up);
            frame[2] *= scale.z;
        }

        WorldCollider world;
        world.entity = entity;
        world.geometry = collider;
        world.localToWorld = glm::translate(glm::dmat4(frame), glm::dvec3(collider.offset));
        return world;
    }

    inline glm::dvec3 SupportLocal(const Components::ColliderComponent &collider, const glm::dvec3 &direction) {
        using Shape = Components::ColliderShape;

        switch (collider.shape) {
            case Shape::Box: {
                glm::dvec3 point(0.0);
                for (int i = 0; i < 3; ++i) {
                    point[i] = collider.size[i] * (direction[i] >= 0.0 ? 0.5 : -0.5);
                }
                return point;
            }
            case Shape::Sphere: {
                const double length = glm::length(direction);
                if (length == 0.0) {
                    return {collider.radius, 0.0, 0.0};
                }
                return direction * (collider.radius / length);
            }
            case Shape::Cylinder: {
                glm::dvec3 point(0.0);
                point.y = direction.y >= 0.0 ? collider.height * 0.5 : -collider.height * 0.5;
                const double radLen = std::hypot(direction.x, direction.z);
                if (radLen > 0.0) {
                    point.x = collider.radius * direction.x / radLen;
                    point.z = collider.radius * direction.z / radLen;
                }
                return point;
            }
            case Shape::Rectangle: {
                return {collider.size.x * (direction.x >= 0.0 ? 0.5 : -0.5), collider.size.y * (direction.y >= 0.0 ? 0.5 : -0.5), 0.0};
            }
            case Shape::Circle: {
                const double radLen = std::hypot(direction.x, direction.y);
                if (radLen == 0.0) {
                    return {collider.radius, 0.0, 0.0};
                }
                return {collider.radius * direction.x / radLen, collider.radius * direction.y / radLen, 0.0};
            }
        }
        return glm::dvec3(0.0);
    }


    inline glm::dvec3 SupportWorld(const WorldCollider &collider, const glm::dvec3 &direction) {
        const glm::dvec3 localDirection = glm::transpose(glm::dmat3(collider.localToWorld)) * direction;
        const glm::dvec3 localPoint = SupportLocal(collider.geometry, localDirection);
        return glm::dvec3(collider.localToWorld * glm::dvec4(localPoint, 1.0));
    }

    struct ColliderAABB {
        glm::dvec3 min{0.0};
        glm::dvec3 max{0.0};
    };

    // world space bounds
    inline ColliderAABB ComputeWorldAABB(const WorldCollider &collider) {
        ColliderAABB aabb;

        for (int i = 0; i < 3; ++i) {
            glm::dvec3 direction(0.0);
            direction[i] = 1.0;

            aabb.max[i] = SupportWorld(collider, direction)[i];
            aabb.min[i] = SupportWorld(collider, -direction)[i];
        }

        return aabb;
    }

    inline bool OverlapsAABB(const ColliderAABB &a, const ColliderAABB &b, const double tolerance = CollisionTolerance) {
        for (int axis = 0; axis < 3; ++axis) {
            if (a.min[axis] > b.max[axis] + tolerance || b.min[axis] > a.max[axis] + tolerance) {
                return false;
            }
        }
        return true;
    }

} // namespace ECS::Collision
