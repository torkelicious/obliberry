#pragma once
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Registry.h"
#include "ECS/Types.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <glm/glm.hpp>
#include <vector>

namespace ECS::Systems::CollisionSystem {

    struct Collision {
        EntityID entityA = INVALID_ENTITY_ID;
        EntityID entityB = INVALID_ENTITY_ID;

        glm::vec2 normal{0.0f};

        float penetration = 0.0f;
        bool isTrigger = false;
    };

    struct WorldCollider {
        ECS::EntityID entity;
        Components::ColliderShape shape;
        glm::vec2 center{0.0f};
        glm::vec2 halfSize{0.5f};
        float radius = 0.5f; // circles
        bool isTrigger = false;
    };

    struct Manifold {
        glm::vec2 normal{0.0f};
        float penetration = 0.0f;
    };

    inline float NonZeroSign(float value) { return value >= 0.0f ? 1.0f : -1.0f; }

    inline WorldCollider makeWorldCollider(ECS::EntityID entity, const Components::ColliderComponent &collider, const Components::TransformComponent &transform) {
        const glm::vec3 position = transform.worldTransform.GetPosition();
        const glm::vec3 scale3 = transform.worldTransform.GetScale();
        const glm::vec2 scale{std::abs(scale3.x), std::abs(scale3.y)};

        WorldCollider result;
        result.entity = entity;
        result.shape = collider.shape;
        result.center = glm::vec2(position) + collider.offset * scale;
        result.halfSize = collider.size * scale * 0.5f;
        result.radius = collider.radius * std::max(scale.x, scale.y);
        result.isTrigger = collider.isTrigger;

        return result;
    }

    inline bool boxVBox(const WorldCollider &a, const WorldCollider &b, Manifold &res) {
        const glm::vec2 delta = b.center - a.center;
        const float overlapX = a.halfSize.x + b.halfSize.x - std::abs(delta.x);

        if (overlapX <= 0.0f) {
            return false;
        }

        const float overlapY = a.halfSize.y + b.halfSize.y - std::abs(delta.y);

        if (overlapY <= 0.0f) {
            return false;
        }

        if (overlapX < overlapY) {
            res.normal = {NonZeroSign(delta.x), 0};
            res.penetration = overlapX;
        } else {
            res.normal = {0.0f, NonZeroSign(delta.y)};
            res.penetration = overlapY;
        }
        return true;
    }

    inline bool circleVCircle(const WorldCollider &a, const WorldCollider &b, Manifold &res) {
        const glm::vec2 delta = b.center - a.center;
        const float radiusSum = a.radius + b.radius;
        const float distSqred = glm::dot(delta, delta);

        if (distSqred >= radiusSum * radiusSum) {
            return false;
        }

        if (distSqred == 0.0f) {
            res.normal = {1.0f, 0.0f};
            res.penetration = radiusSum;
            return true;
        }
        const float distance = std::sqrt(distSqred);
        res.normal = delta / distance;
        res.penetration = radiusSum - distance;
        return true;
    }

    inline bool boxVCircle(const WorldCollider &box, const WorldCollider &circ, Manifold &res) {
        const glm::vec2 boxMin = box.center - box.halfSize;
        const glm::vec2 boxMax = box.center + box.halfSize;

        glm::vec2 closestPoint = glm::clamp(circ.center, boxMin, boxMax);

        const glm::vec2 delta = circ.center - closestPoint;
        const float distSqred = glm::dot(delta, delta);

        if (distSqred > 0.0f) {
            if (distSqred >= circ.radius * circ.radius) {
                return false;
            }
            const float dist = std::sqrt(distSqred);
            res.normal = delta / dist;
            res.penetration = circ.radius - dist;
            return true;
        }

        // circ centre inside box
        const glm::vec2 local = circ.center - box.center;
        const float distToX = box.halfSize.x - std::abs(local.x);
        const float distToY = box.halfSize.y - std::abs(local.y);

        if (distToX < distToY) {
            res.normal = {NonZeroSign(local.x), 0.0f};
            res.penetration = circ.radius + distToX;
        } else {
            res.normal = {0.0f, NonZeroSign(local.y)};
            res.penetration = circ.radius + distToY;
        }
        return true;
    }


    inline bool testCollision(const WorldCollider &a, const WorldCollider &b, Manifold &res) {
        if (a.shape == Components::ColliderShape::Box && b.shape == Components::ColliderShape::Box) {
            return boxVBox(a, b, res);
        }
        if (a.shape == Components::ColliderShape::Circle && b.shape == Components::ColliderShape::Circle) {
            return circleVCircle(a, b, res);
        }
        if (a.shape == Components::ColliderShape::Box && b.shape == Components::ColliderShape::Circle) {
            return boxVCircle(a, b, res);
        }
        // circle versus box
        //  box circle and reverse the normal.
        const bool collided = boxVCircle(b, a, res);
        if (collided) {
            res.normal = -res.normal;
        }
        return collided;
    }


    inline void Update(Registry &reg, std::vector<Collision> &outCollisions) {
        std::vector<WorldCollider> colliders;
        reg.ForEach<Components::ColliderComponent, Components::TransformComponent>([&](const Entity entity, const Components::ColliderComponent *collider, const Components::TransformComponent *transform) {
            colliders.push_back(makeWorldCollider(static_cast<EntityID>(entity), *collider, *transform));
        });

        outCollisions.clear();
        for (size_t i = 0; i < colliders.size(); ++i) {
            for (size_t j = i + 1; j < colliders.size(); ++j) {
                const WorldCollider &a = colliders[i];
                const WorldCollider &b = colliders[j];
                Manifold manifold;

                if (!testCollision(a, b, manifold)) {
                    continue;
                }

                outCollisions.push_back({.entityA = a.entity, .entityB = b.entity, .normal = manifold.normal, .penetration = manifold.penetration, .isTrigger = a.isTrigger || b.isTrigger});
            }
        }
    }

} // namespace ECS::Systems::CollisionSystem
