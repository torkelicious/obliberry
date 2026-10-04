#include "gtest/gtest.h"
#include <array>
#include <gtest/gtest.h>

#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Registry.h"
#include "ECS/Systems/Collision/ColliderGeometry.h"
#include "ECS/Systems/Collision/CollisionFilter.h"
#include "ECS/Systems/Collision/CollisionWorld.h"
#include "ECS/Systems/Collision/GJK.h"
#include "ECS/Systems/HierarchySystem.h"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <tuple>

namespace {
    using Shape = ECS::Components::ColliderShape;
    constexpr std::array colliderTestShapes{Shape::Box, Shape::Sphere, Shape::Cylinder, Shape::Rectangle, Shape::Circle};
} // namespace


TEST(ColliderTests, BothMasksMustAcceptTheOtherLayer) {
    ECS::Components::ColliderComponent a;
    ECS::Components::ColliderComponent b;

    a.layer = 2;
    b.layer = 3;

    a.mask = 1u << 3;
    b.mask = 1u << 2;

    EXPECT_TRUE(ECS::Collision::shouldCollide(a, b));
    EXPECT_TRUE(ECS::Collision::shouldCollide(b, a));

    b.mask = 0;

    EXPECT_FALSE(ECS::Collision::shouldCollide(a, b));
    EXPECT_FALSE(ECS::Collision::shouldCollide(b, a));
}

class CollisionWorldTests : public testing::Test {
protected:
    using Type = ECS::Collision::CollisionEventType;

    struct ExpectedEvent {
        Type type;
        bool isTrigger = false;
    };

    ECS::Registry registry;
    ECS::Collision::CollisionWorld world;

    ECS::EntityID first = ECS::INVALID_ENTITY_ID;
    ECS::EntityID second = ECS::INVALID_ENTITY_ID;

    void SetUp() override {
        first = CreateBox({0.0f, 0.0f, 0.0f});
        second = CreateBox({0.25f, 0.1f, 0.15f});
    }

    ECS::EntityID CreateBox(const glm::vec3 &position) {
        const auto id = registry.CreateEntity();

        registry.AddComponent<ECS::Components::TransformComponent>(id).transform.SetPosition(position);

        registry.AddComponent<ECS::Components::ColliderComponent>(id);

        return id;
    }

    void Update() {
        ECS::Systems::HierarchySystem::Propagate(registry);
        world.Update(registry);
    }

    void ExpectEvents(std::initializer_list<ExpectedEvent> expected) {
        const auto &events = world.GetEvents();
        ASSERT_EQ(events.size(), expected.size());

        std::size_t index = 0;

        for (const auto &wanted : expected) {
            SCOPED_TRACE(index);
            const auto &actual = events[index++];

            EXPECT_EQ(actual.type, wanted.type);
            EXPECT_EQ(actual.isTrigger, wanted.isTrigger);
            EXPECT_EQ(actual.entityA, std::min(first, second));
            EXPECT_EQ(actual.entityB, std::max(first, second));
        }
    }
};

TEST_F(CollisionWorldTests, EnterStayExitLifecycle) {
    Update();
    ExpectEvents({{Type::Enter}});
    EXPECT_EQ(world.GetCollisions().size(), 1u);

    Update();
    ExpectEvents({{Type::Stay}});

    auto *transform = registry.GetComponent<ECS::Components::TransformComponent>(second);

    ASSERT_NE(transform, nullptr);
    transform->transform.SetPosition({4.0f, 0.0f, 0.0f});

    Update();
    ExpectEvents({{Type::Exit}});
    EXPECT_TRUE(world.GetCollisions().empty());

    Update();
    ExpectEvents({});
}

// Triggers still generate collision events.
TEST_F(CollisionWorldTests, TriggerOverlapProducesTriggerEvent) {
    auto *collider = registry.GetComponent<ECS::Components::ColliderComponent>(second);

    ASSERT_NE(collider, nullptr);
    collider->isTrigger = true;

    Update();
    ExpectEvents({{Type::Enter, true}});

    Update();
    ExpectEvents({{Type::Stay, true}});
}

TEST_F(CollisionWorldTests, RejectedMaskEndsExistingCollision) {
    Update();
    ExpectEvents({{Type::Enter}});

    auto *collider = registry.GetComponent<ECS::Components::ColliderComponent>(second);

    ASSERT_NE(collider, nullptr);
    collider->mask = 0;

    Update();
    ExpectEvents({{Type::Exit}});
    EXPECT_TRUE(world.GetCollisions().empty());

    Update();
    ExpectEvents({});
}

TEST_F(CollisionWorldTests, DestroyedEntityProducesExitOnce) {
    Update();
    ExpectEvents({{Type::Enter}});

    registry.DestroyEntity(second);

    Update();
    ExpectEvents({{Type::Exit}});
    EXPECT_TRUE(world.GetCollisions().empty());

    Update();
    ExpectEvents({});
}

TEST_F(CollisionWorldTests, ClearResetsCollisionHistory) {
    Update();
    ExpectEvents({{Type::Enter}});

    world.Clear();

    EXPECT_TRUE(world.GetEvents().empty());
    EXPECT_TRUE(world.GetCollisions().empty());

    Update();
    ExpectEvents({{Type::Enter}});
}

TEST_F(CollisionWorldTests, TriggerChangeProducesExitThenEnter) {
    Update();
    ExpectEvents({{Type::Enter}});

    auto *collider = registry.GetComponent<ECS::Components::ColliderComponent>(second);

    ASSERT_NE(collider, nullptr);
    collider->isTrigger = true;

    Update();
    ExpectEvents({{Type::Exit, false}, {Type::Enter, true}});

    Update();
    ExpectEvents({{Type::Stay, true}});
}

class ColliderGeometryTests : public testing::TestWithParam<std::tuple<Shape, Shape>> {
protected:
    using Result = ECS::Collision::GJKResult;

    ECS::Collision::WorldCollider CreateCollider(const Shape shape, const glm::vec3 &position) {
        ECS::Components::ColliderComponent collider;
        collider.shape = shape;

        ECS::Components::TransformComponent transform;
        transform.worldTransform.SetPosition(position);

        return ECS::Collision::BuildWorldCollider(ECS::INVALID_ENTITY_ID, collider, transform, {});
    }

    void ExpectResult(const glm::vec3 &secondPosition, const Result expected) {
        const auto &[firstShape, secondShape] = GetParam();
        const auto first = CreateCollider(firstShape, {0.0f, 0.0f, 0.0f});
        const auto second = CreateCollider(secondShape, secondPosition);
        EXPECT_EQ(ECS::Collision::IntersectsGJK(first, second), expected);
    }

    void ExpectContactAtGap(const double gap, const Result expected) {
        const float distance = static_cast<float>(1.0 + gap);
        for (const float dir : {-1.0f, 1.0f}) {
            SCOPED_TRACE(dir);
            ExpectResult({dir * distance, 0.0f, 0.0f}, expected);
        }
    }
};

TEST_P(ColliderGeometryTests, OverlappingShapesIntersect) { ExpectResult({0.2f, 0.1f, 0.0f}, Result::Intersecting); }

TEST_P(ColliderGeometryTests, SeparatedShapesDoNotIntersect) {
    for (int axis = 0; axis < 3; ++axis) {
        SCOPED_TRACE(axis);
        glm::vec3 position{0.0f};
        position[axis] = 3.0f;

        ExpectResult(position, Result::Separated);
    }
}

TEST_P(ColliderGeometryTests, TouchingShapesIntersect) { ExpectContactAtGap(-2.0 * ECS::Collision::CollisionTolerance, Result::Intersecting); }
TEST_P(ColliderGeometryTests, GapWithinToleranceConsideredIntersect) { ExpectContactAtGap(0.5 * ECS::Collision::CollisionTolerance, Result::Intersecting); }
TEST_P(ColliderGeometryTests, GapOutsideToleranceConsideredSepareted) { ExpectContactAtGap(2.0 * ECS::Collision::CollisionTolerance, Result::Separated); }

INSTANTIATE_TEST_SUITE_P(AllShapePairs, ColliderGeometryTests, testing::Combine(testing::ValuesIn(colliderTestShapes), testing::ValuesIn(colliderTestShapes)));
