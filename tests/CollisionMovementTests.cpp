#include <array>
#include <gtest/gtest.h>

#include "ECS/Registry.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/Collision/ColliderGeometry.h"
#include "ECS/Systems/Collision/CollisionQueries.h"
#include "ECS/Types.h"
#include "TestingUtils.h"
#include "glm/ext/vector_float3.hpp"

// This file will tests movement that works with colliders and vice-versa
// Which is why these are not part of ColliderTests.cpp

static const glm::vec3 destination{2.0f, 0.0f, 0.0f};

class CollisionMovementTests : public testing::Test {
protected:
    ECS::Registry registry;
    ECS::Collision::BillboardBasis basis{};
    ECS::EntityID entity = ECS::INVALID_ENTITY_ID;

    void SetUp() override { entity = CreateBox({0.0f, 0.0f, 0.0f}); }

    ECS::EntityID CreateBox(const glm::vec3 &position) {
        const ECS::EntityID id = registry.CreateEntity();
        registry.AddComponent<ECS::Components::TransformComponent>(id).transform.SetPosition(position);
        registry.AddComponent<ECS::Components::ColliderComponent>(id);
        return id;
    }

    void ExpectPosition(const glm::vec3 &expected = destination) {
        const auto *comp = registry.GetComponent<ECS::Components::TransformComponent>(entity);
        ASSERT_NE(comp, nullptr);

        const glm::vec3 actual = comp->transform.GetPosition();

        TestUtils::GLM_VecExpectFloat(actual, expected);
    }

    bool TryMove(const glm::vec3 &target = destination) { return ECS::Collision::TryMoveTo(registry, entity, target, basis); }
};

// Check that we can move to a non-blocked position
TEST_F(CollisionMovementTests, EmptyDestinationSucceeds) {
    const bool moved = ECS::Collision::TryMoveTo(registry, entity, destination, basis);

    EXPECT_TRUE(moved);
    ExpectPosition(destination);
}

// Check that a collider properly block movement attempts
TEST_F(CollisionMovementTests, ColliderBlocksMovement) {
    CreateBox(destination);
    const bool moved = ECS::Collision::TryMoveTo(registry, entity, destination, basis);

    EXPECT_FALSE(moved);
    ExpectPosition({0.0f, 0.0f, 0.0f});
}

// Check that triggers do not block movement
TEST_F(CollisionMovementTests, TriggerAllowMove) {
    const ECS::EntityID obstacle = CreateBox(destination);
    auto *collider = registry.GetComponent<ECS::Components::ColliderComponent>(obstacle);
    ASSERT_NE(collider, nullptr);
    collider->isTrigger = true;
    const bool moved = ECS::Collision::TryMoveTo(registry, entity, destination, basis);

    EXPECT_TRUE(moved);
    ExpectPosition(destination);
}

// A rejected mask should let you pass through
TEST_F(CollisionMovementTests, RejectedMaskAllowsMove) {
    const ECS::EntityID obstacle = CreateBox(destination);
    auto *collider = registry.GetComponent<ECS::Components::ColliderComponent>(obstacle);
    ASSERT_NE(collider, nullptr);

    collider->mask = 0;

    const bool moved = ECS::Collision::TryMoveTo(registry, entity, destination, basis);
    EXPECT_TRUE(moved);
    ExpectPosition(destination);
}

// Invalid obstacle collisions should be ignored
TEST_F(CollisionMovementTests, InvalidObstacleIgnored) {
    const ECS::EntityID obstacle = CreateBox(destination);
    auto *collider = registry.GetComponent<ECS::Components::ColliderComponent>(obstacle);
    ASSERT_NE(collider, nullptr);

    collider->size.x = 0.0f; // invalid collider

    ASSERT_FALSE(ECS::Components::IsValidCollider(*collider));

    const bool moved = ECS::Collision::TryMoveTo(registry, entity, destination, basis);

    EXPECT_TRUE(moved);
    ExpectPosition(destination);
}

TEST_F(CollisionMovementTests, InvalidHandleIsRejected) {
    const std::array<ECS::EntityID, 2> invalidhandles{ECS::INVALID_ENTITY_ID, static_cast<ECS::EntityID>(ECS::MAX_ENTITIES)};
    for (const auto handle : invalidhandles) {
        SCOPED_TRACE(handle);
        EXPECT_FALSE(ECS::Collision::TryMoveTo(registry, handle, destination, basis));
        ExpectPosition({0.0f, 0.0f, 0.0f});
    }
}

TEST_F(CollisionMovementTests, DestroyedEntityCannotMove) {
    registry.DestroyEntity(entity);
    ASSERT_FALSE(registry.IsValid(entity));
    EXPECT_FALSE(TryMove());
    EXPECT_EQ(registry.GetComponent<ECS::Components::TransformComponent>(entity), nullptr);
}


TEST_F(CollisionMovementTests, MissingTransformRejectsMovement) {
    registry.RemoveComponent<ECS::Components::TransformComponent>(entity);

    EXPECT_FALSE(TryMove());
    EXPECT_TRUE(registry.IsValid(entity));

    EXPECT_EQ(registry.GetComponent<ECS::Components::TransformComponent>(entity), nullptr);
}

TEST_F(CollisionMovementTests, EntityWithoutColliderCanOverlapSolid) {
    CreateBox(destination);
    registry.RemoveComponent<ECS::Components::ColliderComponent>(entity);

    EXPECT_TRUE(TryMove());
    ExpectPosition(destination);
}

TEST_F(CollisionMovementTests, MovingTriggerCanOverlapSolid) {
    CreateBox(destination);

    auto *collider = registry.GetComponent<ECS::Components::ColliderComponent>(entity);

    ASSERT_NE(collider, nullptr);
    collider->isTrigger = true;

    EXPECT_TRUE(TryMove());
    ExpectPosition(destination);
}


TEST_F(CollisionMovementTests, InvalidMovingColliderDoesNotBlockMovement) {
    CreateBox(destination);
    auto *collider = registry.GetComponent<ECS::Components::ColliderComponent>(entity);
    ASSERT_NE(collider, nullptr);
    collider->size.x = 0.0f;

    ASSERT_FALSE(ECS::Components::IsValidCollider(*collider));

    EXPECT_TRUE(TryMove());
    ExpectPosition(destination);
}

TEST_F(CollisionMovementTests, NonfiniteDestinationsPreservePosition) {
    const std::array<float, 3> invalidValues{std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()};

    for (glm::length_t axis = 0; axis < 3; ++axis) {
        for (const float value : invalidValues) {
            SCOPED_TRACE(testing::Message() << "axis: " << axis << ", value: " << value);

            glm::vec3 target = destination;
            target[axis] = value;

            EXPECT_FALSE(TryMove(target));
            ExpectPosition({0.0f, 0.0f, 0.0f});
        }
    }
}

TEST_F(CollisionMovementTests, ParentedEntityCannotMoveIndependently) {
    const auto parent = CreateBox({10.0f, 0.0f, 0.0f});

    ECS::Systems::HierarchySystem::Propagate(registry);
    registry.Reparent(entity, parent);
    ECS::Systems::HierarchySystem::Propagate(registry);

    const auto *transform = registry.GetComponent<ECS::Components::TransformComponent>(entity);

    ASSERT_NE(transform, nullptr);

    const glm::vec3 localBefore = transform->transform.GetPosition();
    const glm::mat4 worldBefore = transform->worldTransform.GetMatrix();

    EXPECT_FALSE(TryMove());

    ExpectPosition(localBefore);
    TestUtils::GLM_MatExpectNear(transform->worldTransform.GetMatrix(), worldBefore);

    const auto *relationship = registry.GetComponent<ECS::Components::RelationshipComponent>(entity);

    ASSERT_NE(relationship, nullptr);
    EXPECT_EQ(relationship->parent, parent);
}

TEST_F(CollisionMovementTests, SuccessfulMovementUpdatesWorldTransform) {
    ASSERT_TRUE(TryMove());

    ExpectPosition(destination);

    const auto *transform = registry.GetComponent<ECS::Components::TransformComponent>(entity);

    ASSERT_NE(transform, nullptr);

    TestUtils::GLM_VecExpectFloat(transform->worldTransform.GetPosition(), destination);
}
