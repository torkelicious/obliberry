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
