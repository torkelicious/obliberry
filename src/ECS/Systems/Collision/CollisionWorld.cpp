#include "CollisionWorld.h"
#include "ECS/Systems/Collision/CollisionFilter.h"

#define LOG_WHO "Collision"

#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/Systems/Collision/ColliderGeometry.h"
#include "ECS/Systems/Collision/GJK.h"
#include "Logger/LoggerService.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ECS::Collision {

    namespace {
        // Broadphase
        std::vector<std::pair<size_t, size_t>> CollectCandidatePairs(const std::vector<ColliderAABB> &aabbs) {
            std::vector<std::pair<size_t, size_t>> pairs;
            const size_t count = aabbs.size();
            if (count < 2) {
                return pairs;
            }

            double cellSize = 0.0;
            for (const ColliderAABB &aabb : aabbs) {
                for (int axis = 0; axis < 3; ++axis) {
                    cellSize = std::max(cellSize, aabb.max[axis] - aabb.min[axis]);
                }
            }
            if (!std::isfinite(cellSize) || cellSize <= 0.0) {
                cellSize = 1.0;
            }

            struct CellHash {
                size_t operator()(const std::tuple<int64_t, int64_t, int64_t> &cell) const {
                    const auto [x, y, z] = cell;
                    size_t h = std::hash<int64_t>{}(x);
                    h ^= std::hash<int64_t>{}(y) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
                    h ^= std::hash<int64_t>{}(z) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
                    return h;
                }
            };

            std::unordered_map<std::tuple<int64_t, int64_t, int64_t>, std::vector<size_t>, CellHash> grid;

            const auto cellOf = [&cellSize](const double v) { return static_cast<int64_t>(std::floor(v / cellSize)); };

            for (size_t i = 0; i < count; ++i) {
                ColliderAABB aabb = aabbs[i];
                // degenerate geometry
                for (int axis = 0; axis < 3; ++axis) {
                    if (!std::isfinite(aabb.min[axis]) || !std::isfinite(aabb.max[axis])) {
                        aabb.min = glm::dvec3(0.0);
                        aabb.max = glm::dvec3(0.0);
                        break;
                    }
                }

                const int64_t minX = cellOf(aabb.min[0] - CollisionTolerance);
                const int64_t minY = cellOf(aabb.min[1] - CollisionTolerance);
                const int64_t minZ = cellOf(aabb.min[2] - CollisionTolerance);
                const int64_t maxX = cellOf(aabb.max[0] + CollisionTolerance);
                const int64_t maxY = cellOf(aabb.max[1] + CollisionTolerance);
                const int64_t maxZ = cellOf(aabb.max[2] + CollisionTolerance);

                for (int64_t x = minX; x <= maxX; ++x) {
                    for (int64_t y = minY; y <= maxY; ++y) {
                        for (int64_t z = minZ; z <= maxZ; ++z) {
                            for (const size_t other : grid[std::make_tuple(x, y, z)]) {
                                pairs.emplace_back(std::min(other, i), std::max(other, i));
                            }
                            grid[std::make_tuple(x, y, z)].push_back(i);
                        }
                    }
                }
            }

            std::sort(pairs.begin(), pairs.end());
            pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
            return pairs;
        }
    } // namespace

    void CollisionWorld::Update(Registry &registry, const BillboardBasis &basis) {
        m_Events.clear();
        std::vector<WorldCollider> worldColliders;

        registry.ForEach<Components::ColliderComponent, Components::TransformComponent>([&](const Entity entity, const Components::ColliderComponent *collider, const Components::TransformComponent *transform) {
            const auto entityID = static_cast<EntityID>(entity);

            const bool valid = Components::IsValidCollider(*collider);
            ReportInvalidCollider(entityID, valid);
            if (!valid) {
                return;
            }


            worldColliders.push_back(BuildWorldCollider(entityID, *collider, *transform, basis));
        });

        std::vector<ColliderAABB> aabbs;
        aabbs.reserve(worldColliders.size());

        for (const WorldCollider &collider : worldColliders) {
            aabbs.push_back(ComputeWorldAABB(collider));
        }

        std::vector<Collision> nextCollisions;
        for (const auto &[i, j] : CollectCandidatePairs(aabbs)) {
            const auto &a = worldColliders[i];
            const auto &b = worldColliders[j];

            if (!shouldCollide(a.geometry, b.geometry)) {
                continue;
            }

            if (!OverlapsAABB(aabbs[i], aabbs[j])) {
                continue;
            }

            const GJKResult result = IntersectsGJK(a, b);

            ReportIndeterminate(a.entity, b.entity, result == GJKResult::Indeterminate);
            if (result == GJKResult::Indeterminate) {
                const auto previous = m_Previous.find(MakeKey(a.entity, b.entity));
                if (previous != m_Previous.end()) {
                    nextCollisions.push_back(previous->second);
                }
                continue;
            }

            if (result == GJKResult::Separated) {
                continue;
            }

            nextCollisions.push_back({.entityA = std::min(a.entity, b.entity), .entityB = std::max(a.entity, b.entity), .isTrigger = a.geometry.isTrigger || b.geometry.isTrigger});
        }
        CollisionMap nextPairs;
        nextPairs.reserve(nextCollisions.size());

        for (const Collision &collision : nextCollisions) {
            const uint64_t key = MakeKey(collision.entityA, collision.entityB);
            const auto previous = m_Previous.find(key);

            if (previous == m_Previous.end()) {
                Emit(CollisionEventType::Enter, collision);
            } else if (previous->second.isTrigger != collision.isTrigger) {
                Emit(CollisionEventType::Exit, previous->second);
                Emit(CollisionEventType::Enter, collision);
            } else {
                Emit(CollisionEventType::Stay, collision);
            }
            nextPairs.emplace(key, collision);
        }
        for (const auto &[key, previous] : m_Previous) {
            if (!nextPairs.contains(key)) {
                Emit(CollisionEventType::Exit, previous);
            }
        }
        m_Previous = std::move(nextPairs);
        m_Collisions = std::move(nextCollisions);
    }

    const std::vector<CollisionEvent> &CollisionWorld::GetEvents() const { return m_Events; }

    const std::vector<Collision> &CollisionWorld::GetCollisions() const { return m_Collisions; }

    void CollisionWorld::Clear() {
        m_Previous.clear();
        m_Collisions.clear();
        m_Events.clear();
        m_ReportedInvalid.clear();
        m_ReportedIndeterminate.clear();
    }

    uint64_t CollisionWorld::MakeKey(EntityID a, EntityID b) {
        if (a > b) {
            std::swap(a, b);
        }
        return (static_cast<uint64_t>(a) << 32) | static_cast<uint64_t>(b);
    }

    void CollisionWorld::Emit(const CollisionEventType type, const Collision &collision) {
        m_Events.push_back({.type = type, .entityA = collision.entityA, .entityB = collision.entityB, .isTrigger = collision.isTrigger});
    }

    void CollisionWorld::ReportInvalidCollider(const EntityID entity, const bool valid) {
        if (!valid && m_ReportedInvalid.insert(entity).second) {
            LOG_ERROR(LOG_WHO, "Collision skipped entity with invalid collider: " + std::to_string(entity));
        } else if (valid) {
            m_ReportedInvalid.erase(entity);
        }
    }

    void CollisionWorld::ReportIndeterminate(const EntityID a, const EntityID b, const bool indeterminate) {
        const uint64_t key = MakeKey(a, b);

        if (!indeterminate) {
            m_ReportedIndeterminate.erase(key);
        } else if (m_ReportedIndeterminate.insert(key).second) {
            LOG_ERROR(LOG_WHO, "GJK was indeterminate for pair " + std::to_string(a) + " / " + std::to_string(b) + ", preserving previous pair state");
        }
    }

} // namespace ECS::Collision
