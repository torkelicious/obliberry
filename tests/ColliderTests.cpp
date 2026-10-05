#include <gtest/gtest.h>

#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Registry.h"
#include "ECS/Systems/Collision/ColliderGeometry.h"
#include "ECS/Systems/Collision/CollisionFilter.h"
#include "ECS/Systems/Collision/CollisionQueries.h"
#include "ECS/Systems/Collision/CollisionWorld.h"
#include "ECS/Systems/Collision/GJK.h"
#include "ECS/Systems/HierarchySystem.h"
#include "TestingUtils.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {
    using Shape = ECS::Components::ColliderShape;
    using Collider = ECS::Components::ColliderComponent;
    using Orientation = ECS::Components::ColliderOrientation;
    using Basis = ECS::Collision::BillboardBasis;

    constexpr Basis rotatedBillboardBasis{{0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}};

    const char *ShapeName(const Shape shape) {
        switch (shape) {
            case Shape::Box:
                return "Box";
            case Shape::Sphere:
                return "Sphere";
            case Shape::Cylinder:
                return "Cylinder";
            case Shape::Rectangle:
                return "Rectangle";
            case Shape::Circle:
                return "Circle";
        }
        return "Unknown";
    }

    enum class Dimension { SizeX, SizeY, SizeZ, Radius, Height };

    constexpr std::array colliderDimensions{Dimension::SizeX, Dimension::SizeY, Dimension::SizeZ, Dimension::Radius, Dimension::Height};

    float &DimensionValue(Collider &collider, const Dimension dimension) {
        switch (dimension) {
            case Dimension::SizeX:
                return collider.size.x;
            case Dimension::SizeY:
                return collider.size.y;
            case Dimension::SizeZ:
                return collider.size.z;
            case Dimension::Radius:
                return collider.radius;
            case Dimension::Height:
                return collider.height;
        }
        return collider.radius;
    }

    bool UsesDimension(const Shape shape, const Dimension dimension) {
        switch (shape) {
            case Shape::Box:
                return dimension == Dimension::SizeX || dimension == Dimension::SizeY || dimension == Dimension::SizeZ;
            case Shape::Rectangle:
                return dimension == Dimension::SizeX || dimension == Dimension::SizeY;
            case Shape::Sphere:
            case Shape::Circle:
                return dimension == Dimension::Radius;
            case Shape::Cylinder:
                return dimension == Dimension::Radius || dimension == Dimension::Height;
        }
        return false;
    }

    const std::array invalidDimensionValues{0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()};

    const std::array nonFiniteValues{std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()};
    constexpr std::array colliderTestShapes{Shape::Box, Shape::Sphere, Shape::Cylinder, Shape::Rectangle, Shape::Circle};
} // namespace


class ColliderGeometryTests : public testing::TestWithParam<std::tuple<Shape, Shape>> {
protected:
    using Result = ECS::Collision::GJKResult;

    ECS::Collision::WorldCollider CreateCollider(const Shape shape, const glm::vec3 &position, const glm::vec3 &offset = glm::vec3{0.0f}, const glm::vec3 &rotation = glm::vec3{0.0f},
            const glm::vec3 &scale = glm::vec3{1.0f}, const ECS::Components::ColliderOrientation orientation = ECS::Components::ColliderOrientation::Entity, const ECS::Collision::BillboardBasis &basis = {}) {
        ECS::Components::ColliderComponent collider;
        collider.shape = shape;
        collider.offset = offset;
        collider.orientation = orientation;
        ECS::Components::TransformComponent transform;
        transform.worldTransform.SetPosition(position);
        transform.worldTransform.SetRotation(rotation);
        transform.worldTransform.SetScale(scale);
        return ECS::Collision::BuildWorldCollider(ECS::INVALID_ENTITY_ID, collider, transform, basis);
    }

    void ExpectResult(
            const glm::vec3 &secondPosition, const Result expected, const glm::vec3 &firstOffset = glm::vec3{0.0f}, const glm::vec3 &firstRotation = glm::vec3{0.0f}, const glm::vec3 &firstScale = glm::vec3{1.0f}) {
        const auto &[firstShape, secondShape] = GetParam();
        const auto first = CreateCollider(firstShape, {0.0f, 0.0f, 0.0f}, firstOffset, firstRotation, firstScale);
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

    void ExpectPointInsideAABB(const ECS::Collision::ColliderAABB &bounds, const glm::dvec3 &point) {
        const double tolerance = ECS::Collision::CollisionTolerance;
        for (int axis = 0; axis < 3; ++axis) {
            SCOPED_TRACE(axis);
            EXPECT_GE(point[axis], bounds.min[axis] - tolerance);
            EXPECT_LE(point[axis], bounds.max[axis] + tolerance);
        }
    }
};

TEST_P(ColliderGeometryTests, SeparatedShapesDoNotIntersect) {
    for (int axis = 0; axis < 3; ++axis) {
        SCOPED_TRACE(axis);
        glm::vec3 position{0.0f};
        position[axis] = 3.0f;

        ExpectResult(position, Result::Separated);
    }
}

TEST_P(ColliderGeometryTests, OverlappingShapesIntersect) { ExpectResult({0.2f, 0.1f, 0.0f}, Result::Intersecting); }
TEST_P(ColliderGeometryTests, TouchingShapesIntersect) { ExpectContactAtGap(0.0, Result::Intersecting); }

TEST_P(ColliderGeometryTests, SlightlyOverlappingShapesIntersect) { ExpectContactAtGap(-2.0 * ECS::Collision::CollisionTolerance, Result::Intersecting); }
TEST_P(ColliderGeometryTests, GapWithinToleranceConsideredIntersect) { ExpectContactAtGap(0.5 * ECS::Collision::CollisionTolerance, Result::Intersecting); }
TEST_P(ColliderGeometryTests, GapOutsideToleranceConsideredSeparated) { ExpectContactAtGap(2.0 * ECS::Collision::CollisionTolerance, Result::Separated); }

TEST_P(ColliderGeometryTests, OffsetCanCreateIntersection) {
    const glm::vec3 position{3.2f, 0.1f, 0.0f};
    ExpectResult(position, Result::Separated);
    ExpectResult(position, Result::Intersecting, {3.0f, 0.0f, 0.0f});
}

TEST_P(ColliderGeometryTests, OffsetCanRemoveIntersection) {
    const glm::vec3 position{0.2f, 0.1f, 0.0f};
    ExpectResult(position, Result::Intersecting);
    ExpectResult(position, Result::Separated, {3.0f, 0.0f, 0.0f});
}

TEST_P(ColliderGeometryTests, ScalingCanCreateIntersection) {
    const glm::vec3 position{2.0f, 0.0f, 0.0f};
    ExpectResult(position, Result::Separated);
    ExpectResult(position, Result::Intersecting, {}, {}, {4.0f, 1.0f, 1.0f});
}

TEST_P(ColliderGeometryTests, ShrinkingCanRemoveIntersection) {
    const glm::vec3 position{0.8f, 0.0f, 0.0f};
    ExpectResult(position, Result::Intersecting);
    ExpectResult(position, Result::Separated, {}, {}, {0.25f, 0.25f, 0.25f});
}

TEST_P(ColliderGeometryTests, RotationChangesCollisionOrientation) {
    const glm::vec3 scale{4.0f, 1.0f, 1.0f};
    const glm::vec3 rotation{0.0f, 0.0f, glm::radians(90.0f)};
    ExpectResult({1.5f, 0.0f, 0.0f}, Result::Intersecting, {}, {}, scale);
    ExpectResult({1.5f, 0.0f, 0.0f}, Result::Separated, {}, rotation, scale);
    ExpectResult({0.0f, 1.5f, 0.0f}, Result::Intersecting, {}, rotation, scale);
}

TEST_P(ColliderGeometryTests, LocalOffsetIsScaledAndRotated) { ExpectResult({0.0f, 2.0f, 0.0f}, Result::Intersecting, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, glm::radians(90.0f)}, {2.0f, 1.0f, 1.0f}); }

TEST_P(ColliderGeometryTests, AABBMatchesTransformedBounds) {
    const auto shape = std::get<0>(GetParam());
    const auto collider = CreateCollider(shape, {3.0f, -2.0f, 5.0f}, {0.5f, -0.25f, 0.75f}, {0.0f, 0.0f, glm::radians(90.0f)}, {2.0f, 4.0f, 6.0f});
    const auto bounds = ECS::Collision::ComputeWorldAABB(collider);
    const glm::dvec3 center{4.0, -1.0, 9.5};
    glm::dvec3 halfExtents{2.0, 1.0, 3.0};

    if (shape == Shape::Rectangle || shape == Shape::Circle) {
        halfExtents.z = 0.0;
    }

    const glm::dvec3 expectedMin = center - halfExtents;
    const glm::dvec3 expectedMax = center + halfExtents;

    for (int axis = 0; axis < 3; ++axis) {
        SCOPED_TRACE(axis);
        EXPECT_NEAR(bounds.min[axis], expectedMin[axis], 1e-5);
        EXPECT_NEAR(bounds.max[axis], expectedMax[axis], 1e-5);
    }
}

TEST_P(ColliderGeometryTests, AABBContainsSupportPointsAfterTransforms) {
    const auto shape = std::get<0>(GetParam());
    const auto collider = CreateCollider(shape, {3.0f, -2.0f, 5.0f}, {0.4f, -0.2f, 0.3f}, {0.3f, -0.7f, 0.9f}, {2.0f, 0.5f, 3.0f});
    const auto bounds = ECS::Collision::ComputeWorldAABB(collider);

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            for (int z = -1; z <= 1; ++z) {
                if (x == 0 && y == 0 && z == 0) {
                    continue;
                }
                SCOPED_TRACE(testing::Message() << "Direction: " << x << ", " << y << ", " << z);
                const glm::dvec3 direction{x, y, z};
                ExpectPointInsideAABB(bounds, ECS::Collision::SupportWorld(collider, direction));
            }
        }
    }
}

TEST_P(ColliderGeometryTests, BillboardUsesCameraBasisForScaleAndOffset) {
    const auto shape = std::get<0>(GetParam());
    const auto &basis = rotatedBillboardBasis;
    const auto collider = CreateCollider(shape, {3.0f, -2.0f, 5.0f}, {0.5f, -0.25f, 0.75f}, {0.3f, -0.7f, 0.9f}, {2.0f, 4.0f, 6.0f}, ECS::Components::ColliderOrientation::Billboard, basis);
    const glm::dmat4 expected{glm::dvec4{0.0, 0.0, -2.0, 0.0}, glm::dvec4{0.0, 4.0, 0.0, 0.0}, glm::dvec4{6.0, 0.0, 0.0, 0.0}, glm::dvec4{7.5, -1.0, 4.0, 1.0}};
    TestUtils::GLM_MatExpectNear(collider.localToWorld, expected);
}

TEST_P(ColliderGeometryTests, ChangingCameraBasisChangesBillboardCollision) {
    const auto &[firstShape, secondShape] = GetParam();
    const auto &rotatedBasis = rotatedBillboardBasis;

    const auto standard = CreateCollider(firstShape, {}, {}, {}, {4.0f, 1.0f, 1.0f}, ECS::Components::ColliderOrientation::Billboard);
    const auto rotated = CreateCollider(firstShape, {}, {}, {}, {4.0f, 1.0f, 1.0f}, ECS::Components::ColliderOrientation::Billboard, rotatedBasis);

    const auto probeX = CreateCollider(secondShape, {1.5f, 0.5f, 0.0f});
    const auto probeZ = CreateCollider(secondShape, {0.0f, 0.5f, 1.5f});

    EXPECT_EQ(ECS::Collision::IntersectsGJK(standard, probeX), Result::Intersecting);
    EXPECT_EQ(ECS::Collision::IntersectsGJK(rotated, probeX), Result::Separated);
    EXPECT_EQ(ECS::Collision::IntersectsGJK(standard, probeZ), Result::Separated);
    EXPECT_EQ(ECS::Collision::IntersectsGJK(rotated, probeZ), Result::Intersecting);
}

TEST_P(ColliderGeometryTests, EntityOrientationIgnoresCameraBasis) {
    const auto shape = std::get<0>(GetParam());
    const auto &rotatedBasis = rotatedBillboardBasis;

    const glm::vec3 position{3.0f, -2.0f, 5.0f};
    const glm::vec3 offset{0.5f, -0.25f, 0.75f};
    const glm::vec3 rotation{0.3f, -0.7f, 0.9f};
    const glm::vec3 scale{2.0f, 4.0f, 6.0f};

    const auto standard = CreateCollider(shape, position, offset, rotation, scale);
    const auto rotated = CreateCollider(shape, position, offset, rotation, scale, ECS::Components::ColliderOrientation::Entity, rotatedBasis);

    TestUtils::GLM_MatExpectNear(rotated.localToWorld, standard.localToWorld);
}

INSTANTIATE_TEST_SUITE_P(AllShapePairs, ColliderGeometryTests, testing::Combine(testing::ValuesIn(colliderTestShapes), testing::ValuesIn(colliderTestShapes)),
        [](const testing::TestParamInfo<std::tuple<Shape, Shape>> &info) { return std::string{ShapeName(std::get<0>(info.param))} + "_" + ShapeName(std::get<1>(info.param)); });

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


TEST(ColliderTests, LayersZeroAndThirtyOneUseTheirCorrectMaskBits) {
    Collider a;
    Collider b;
    a.layer = 0;
    b.layer = 31;
    a.mask = 1u << 31;
    b.mask = 1u;

    EXPECT_TRUE(ECS::Collision::shouldCollide(a, b));
    EXPECT_TRUE(ECS::Collision::shouldCollide(b, a));

    a.mask = 1u;
    EXPECT_FALSE(ECS::Collision::shouldCollide(a, b));
    EXPECT_FALSE(ECS::Collision::shouldCollide(b, a));
}

TEST(ColliderTests, EveryLayerCanFilterEveryOtherLayer) {
    for (unsigned firstLayer = 0; firstLayer < 32; ++firstLayer) {
        for (unsigned secondLayer = 0; secondLayer < 32; ++secondLayer) {
            SCOPED_TRACE(testing::Message() << firstLayer << ", " << secondLayer);

            Collider a;
            Collider b;
            a.layer = static_cast<std::uint8_t>(firstLayer);
            b.layer = static_cast<std::uint8_t>(secondLayer);
            a.mask = 1u << secondLayer;
            b.mask = 1u << firstLayer;

            EXPECT_TRUE(ECS::Collision::shouldCollide(a, b));
            EXPECT_TRUE(ECS::Collision::shouldCollide(b, a));

            b.mask = 0;
            EXPECT_FALSE(ECS::Collision::shouldCollide(a, b));
            EXPECT_FALSE(ECS::Collision::shouldCollide(b, a));
        }
    }
}

TEST(ColliderTests, InvalidLayersAreRejectedBeforeShifting) {
    for (const unsigned layer : {32u, 33u, 255u}) {
        SCOPED_TRACE(layer);

        Collider a;
        Collider b;
        a.layer = static_cast<std::uint8_t>(layer);

        EXPECT_FALSE(ECS::Collision::shouldCollide(a, b));
        EXPECT_FALSE(ECS::Collision::shouldCollide(b, a));
    }
}

TEST(ColliderTests, SameLayerPairsStillRequireBothMasks) {
    Collider a;
    Collider b;
    a.layer = 31;
    b.layer = 31;
    a.mask = 1u << 31;
    b.mask = 1u << 31;

    EXPECT_TRUE(ECS::Collision::shouldCollide(a, b));

    a.mask = 0;
    EXPECT_FALSE(ECS::Collision::shouldCollide(a, b));
    EXPECT_FALSE(ECS::Collision::shouldCollide(b, a));
}

class ColliderValidationTests : public testing::TestWithParam<Shape> {
protected:
    Collider CreateCollider() const {
        Collider collider;
        collider.shape = GetParam();
        return collider;
    }
};

TEST_P(ColliderValidationTests, DefaultDimensionsAreValid) { EXPECT_TRUE(ECS::Components::IsValidCollider(CreateCollider())); }

TEST_P(ColliderValidationTests, RequiredDimensionsMustBePositiveAndFinite) {
    for (const auto dimension : colliderDimensions) {
        if (!UsesDimension(GetParam(), dimension)) {
            continue;
        }

        for (const float value : invalidDimensionValues) {
            SCOPED_TRACE(testing::Message() << "Dimension: " << static_cast<int>(dimension) << ", value: " << value);

            auto collider = CreateCollider();
            DimensionValue(collider, dimension) = value;
            EXPECT_FALSE(ECS::Components::IsValidCollider(collider));
        }
    }
}

TEST_P(ColliderValidationTests, UnusedDimensionsDoNotInvalidateShape) {
    for (const auto dimension : colliderDimensions) {
        if (UsesDimension(GetParam(), dimension)) {
            continue;
        }

        for (const float value : invalidDimensionValues) {
            SCOPED_TRACE(testing::Message() << "Dimension: " << static_cast<int>(dimension) << ", value: " << value);

            auto collider = CreateCollider();
            DimensionValue(collider, dimension) = value;
            EXPECT_TRUE(ECS::Components::IsValidCollider(collider));
        }
    }
}

TEST_P(ColliderValidationTests, EveryOffsetAxisMustBeFinite) {
    for (int axis = 0; axis < 3; ++axis) {
        for (const float value : nonFiniteValues) {
            SCOPED_TRACE(testing::Message() << "Axis: " << axis << ", value: " << value);

            auto collider = CreateCollider();
            collider.offset[axis] = value;
            EXPECT_FALSE(ECS::Components::IsValidCollider(collider));
        }
    }
}

TEST_P(ColliderValidationTests, FiniteNegativeOffsetsAreValid) {
    auto collider = CreateCollider();
    collider.offset = {-3.0f, 2.0f, -1.0f};
    EXPECT_TRUE(ECS::Components::IsValidCollider(collider));
}

TEST_P(ColliderValidationTests, BothOrientationsAreValid) {
    auto collider = CreateCollider();

    for (const auto orientation : {Orientation::Entity, Orientation::Billboard}) {
        collider.orientation = orientation;
        EXPECT_TRUE(ECS::Components::IsValidCollider(collider));
    }

    collider.orientation = static_cast<Orientation>(255);
    EXPECT_FALSE(ECS::Components::IsValidCollider(collider));
}

TEST_P(ColliderValidationTests, LayerBoundsAndZeroMaskAreValidated) {
    auto collider = CreateCollider();
    collider.mask = 0;

    for (const unsigned layer : {0u, 31u}) {
        collider.layer = static_cast<std::uint8_t>(layer);
        EXPECT_TRUE(ECS::Components::IsValidCollider(collider));
    }

    for (const unsigned layer : {32u, 255u}) {
        collider.layer = static_cast<std::uint8_t>(layer);
        EXPECT_FALSE(ECS::Components::IsValidCollider(collider));
    }
}

TEST(ColliderTests, UnknownShapeIsInvalid) {
    Collider collider;
    collider.shape = static_cast<Shape>(255);
    EXPECT_FALSE(ECS::Components::IsValidCollider(collider));
}

INSTANTIATE_TEST_SUITE_P(AllShapes, ColliderValidationTests, testing::ValuesIn(colliderTestShapes), [](const testing::TestParamInfo<Shape> &info) { return std::string{ShapeName(info.param)}; });

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

    static ECS::EntityID CreateBox(ECS::Registry &targetRegistry, const glm::vec3 &position) {
        const auto id = targetRegistry.CreateEntity();
        targetRegistry.AddComponent<ECS::Components::TransformComponent>(id).transform.SetPosition(position);
        targetRegistry.AddComponent<Collider>(id);
        return id;
    }

    ECS::EntityID CreateBox(const glm::vec3 &position) { return CreateBox(registry, position); }

    void Update(const Basis &basis = {}) {
        ECS::Systems::HierarchySystem::Propagate(registry);
        world.Update(registry, basis);
    }

    void ConfigureBillboardPair() {
        auto *collider = registry.GetComponent<Collider>(first);
        auto *firstTransform = registry.GetComponent<ECS::Components::TransformComponent>(first);
        auto *secondTransform = registry.GetComponent<ECS::Components::TransformComponent>(second);

        ASSERT_NE(collider, nullptr);
        ASSERT_NE(firstTransform, nullptr);
        ASSERT_NE(secondTransform, nullptr);

        collider->orientation = Orientation::Billboard;
        firstTransform->transform.SetPosition({0.0f, 0.0f, 0.0f});
        firstTransform->transform.SetScale({4.0f, 1.0f, 1.0f});
        secondTransform->transform.SetPosition({1.5f, 0.5f, 0.0f});
    }

    void ExpectCurrentOccupancy(const bool expected, const Basis &basis = {}) {
        for (const auto id : {first, second}) {
            SCOPED_TRACE(id);
            const auto *transform = registry.GetComponent<ECS::Components::TransformComponent>(id);
            ASSERT_NE(transform, nullptr);

            EXPECT_EQ(ECS::Collision::CanOccupy(registry, id, transform->worldTransform.GetPosition(), basis), expected);
        }
    }

    using Pair = std::pair<ECS::EntityID, ECS::EntityID>;
    using EventRecord = std::tuple<Type, ECS::EntityID, ECS::EntityID, bool>;

    void ExpectCollisionPairs(std::vector<Pair> expected) {
        std::vector<Pair> actual;

        for (const auto &collision : world.GetCollisions()) {
            EXPECT_LT(collision.entityA, collision.entityB);
            EXPECT_FALSE(collision.isTrigger);
            actual.emplace_back(collision.entityA, collision.entityB);
        }

        for (auto &[a, b] : expected) {
            if (a > b) {
                std::swap(a, b);
            }
        }

        std::sort(actual.begin(), actual.end());
        std::sort(expected.begin(), expected.end());
        EXPECT_EQ(actual, expected);
        EXPECT_EQ(std::adjacent_find(actual.begin(), actual.end()), actual.end());
    }

    void ExpectUnorderedEvents(std::vector<EventRecord> expected) {
        std::vector<EventRecord> actual;

        for (const auto &event : world.GetEvents()) {
            EXPECT_LT(event.entityA, event.entityB);
            actual.emplace_back(event.type, event.entityA, event.entityB, event.isTrigger);
        }

        for (auto &[type, a, b, trigger] : expected) {
            if (a > b) {
                std::swap(a, b);
            }
        }

        std::sort(actual.begin(), actual.end());
        std::sort(expected.begin(), expected.end());
        EXPECT_EQ(actual, expected);
        EXPECT_EQ(std::adjacent_find(actual.begin(), actual.end()), actual.end());
    }

    using RolePair = std::pair<int, int>;
    using RoleEvent = std::tuple<Type, int, int, bool>;
    using SceneResult = std::pair<std::vector<RolePair>, std::vector<RoleEvent>>;

    static SceneResult CaptureScene(const std::array<int, 4> &insertionOrder) {
        ECS::Registry sceneRegistry;
        ECS::Collision::CollisionWorld sceneWorld;
        std::array<ECS::EntityID, 4> ids{};
        const std::array positions{glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.25f, 0.1f, 0.15f}, glm::vec3{0.4f, 0.2f, 0.1f}, glm::vec3{4.0f, 0.0f, 0.0f}};

        for (const int role : insertionOrder) {
            ids[role] = CreateBox(sceneRegistry, positions[role]);
        }

        ECS::Systems::HierarchySystem::Propagate(sceneRegistry);
        sceneWorld.Update(sceneRegistry);

        const auto roleOf = [&](const ECS::EntityID id) { return static_cast<int>(std::distance(ids.begin(), std::find(ids.begin(), ids.end(), id))); };

        SceneResult result;

        for (const auto &collision : sceneWorld.GetCollisions()) {
            EXPECT_LT(collision.entityA, collision.entityB);
            EXPECT_FALSE(collision.isTrigger);
            const int a = roleOf(collision.entityA);
            const int b = roleOf(collision.entityB);
            result.first.emplace_back(std::min(a, b), std::max(a, b));
        }

        for (const auto &event : sceneWorld.GetEvents()) {
            EXPECT_LT(event.entityA, event.entityB);
            const int a = roleOf(event.entityA);
            const int b = roleOf(event.entityB);
            result.second.emplace_back(event.type, std::min(a, b), std::max(a, b), event.isTrigger);
        }

        std::sort(result.first.begin(), result.first.end());
        std::sort(result.second.begin(), result.second.end());
        return result;
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


TEST_F(CollisionWorldTests, BillboardQueriesAgreeWithCollisionEvents) {
    ConfigureBillboardPair();

    Update();
    ExpectEvents({{Type::Enter}});
    ExpectCurrentOccupancy(false);

    Update(rotatedBillboardBasis);
    ExpectEvents({{Type::Exit}});
    ExpectCurrentOccupancy(true, rotatedBillboardBasis);
    EXPECT_TRUE(world.GetCollisions().empty());

    Update();
    ExpectEvents({{Type::Enter}});
    ExpectCurrentOccupancy(false);
}

TEST_F(CollisionWorldTests, BillboardMovementUsesSuppliedBasis) {
    ConfigureBillboardPair();

    auto *transform = registry.GetComponent<ECS::Components::TransformComponent>(first);
    ASSERT_NE(transform, nullptr);

    const glm::vec3 start{-4.0f, 0.0f, 0.0f};
    const glm::vec3 destination{0.0f, 0.0f, 0.0f};
    transform->transform.SetPosition(start);

    Update();
    ExpectEvents({});

    EXPECT_FALSE(ECS::Collision::TryMoveTo(registry, first, destination, {}));
    EXPECT_EQ(transform->transform.GetPosition(), start);

    ASSERT_TRUE(ECS::Collision::TryMoveTo(registry, first, destination, rotatedBillboardBasis));
    EXPECT_EQ(transform->transform.GetPosition(), destination);

    Update(rotatedBillboardBasis);
    ExpectEvents({});
    EXPECT_TRUE(world.GetCollisions().empty());

    Update();
    ExpectEvents({{Type::Enter}});
    ExpectCurrentOccupancy(false);
}

TEST_F(CollisionWorldTests, LayersZeroAndThirtyOneBlockAndGenerateEvents) {
    auto *a = registry.GetComponent<Collider>(first);
    auto *b = registry.GetComponent<Collider>(second);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    a->layer = 0;
    b->layer = 31;
    a->mask = 1u << 31;
    b->mask = 1u;

    Update();
    ExpectEvents({{Type::Enter}});
    ExpectCurrentOccupancy(false);
}

TEST_F(CollisionWorldTests, RejectedMasksPermitOccupancyAndProduceNoEvents) {
    auto *collider = registry.GetComponent<Collider>(first);
    ASSERT_NE(collider, nullptr);
    collider->mask = 0;

    Update();
    ExpectEvents({});
    ExpectCurrentOccupancy(true);
    EXPECT_TRUE(world.GetCollisions().empty());

    collider->mask = 0xFFFFFFFFu;
    Update();
    ExpectEvents({{Type::Enter}});
    ExpectCurrentOccupancy(false);
}

TEST_F(CollisionWorldTests, InvalidLayerDoesNotBlockOrGenerateEvents) {
    auto *collider = registry.GetComponent<Collider>(first);
    ASSERT_NE(collider, nullptr);
    collider->layer = 32;

    Update();
    ExpectEvents({});
    ExpectCurrentOccupancy(true);
    EXPECT_TRUE(world.GetCollisions().empty());
}

TEST_F(CollisionWorldTests, MultiplePairsProduceNoDuplicateCollisionsOrEvents) {
    const auto third = CreateBox({0.4f, 0.2f, 0.1f});

    Update();
    ExpectCollisionPairs({{first, second}, {first, third}, {second, third}});
    ExpectUnorderedEvents({{Type::Enter, first, second, false}, {Type::Enter, first, third, false}, {Type::Enter, second, third, false}});

    Update();
    ExpectCollisionPairs({{first, second}, {first, third}, {second, third}});
    ExpectUnorderedEvents({{Type::Stay, first, second, false}, {Type::Stay, first, third, false}, {Type::Stay, second, third, false}});

    auto *transform = registry.GetComponent<ECS::Components::TransformComponent>(third);
    ASSERT_NE(transform, nullptr);
    transform->transform.SetPosition({4.0f, 0.0f, 0.0f});

    Update();
    ExpectCollisionPairs({{first, second}});
    ExpectUnorderedEvents({{Type::Stay, first, second, false}, {Type::Exit, first, third, false}, {Type::Exit, second, third, false}});

    Update();
    ExpectCollisionPairs({{first, second}});
    ExpectUnorderedEvents({{Type::Stay, first, second, false}});
}

TEST_F(CollisionWorldTests, InsertionOrderDoesNotChangeCollisionPairs) {
    const SceneResult expected{{{0, 1}, {0, 2}, {1, 2}}, {{Type::Enter, 0, 1, false}, {Type::Enter, 0, 2, false}, {Type::Enter, 1, 2, false}}};

    const std::array orders{std::array{0, 1, 2, 3}, std::array{3, 2, 1, 0}, std::array{2, 0, 3, 1}};

    for (const auto &order : orders) {
        SCOPED_TRACE(testing::Message() << order[0] << ", " << order[1] << ", " << order[2] << ", " << order[3]);
        EXPECT_EQ(CaptureScene(order), expected);
    }
}

class InvalidColliderWorldTests : public CollisionWorldTests, public testing::WithParamInterface<Shape> {
protected:
    void SetUp() override {
        CollisionWorldTests::SetUp();
        registry.GetComponent<Collider>(first)->shape = GetParam();
        registry.GetComponent<Collider>(second)->shape = GetParam();
        registry.GetComponent<ECS::Components::TransformComponent>(second)->transform.SetPosition({0.2f, 0.1f, 0.0f});
    }

    void ExpectInvalidColliderEndsPair(const Collider &invalid) {
        auto *collider = registry.GetComponent<Collider>(second);
        ASSERT_NE(collider, nullptr);
        const Collider valid = *collider;

        Update();
        ExpectEvents({{Type::Enter}});
        ExpectCurrentOccupancy(false);

        *collider = invalid;
        Update();
        ExpectEvents({{Type::Exit}});
        ExpectCurrentOccupancy(true);
        EXPECT_TRUE(world.GetCollisions().empty());

        Update();
        ExpectEvents({});

        *collider = valid;
        Update();
        ExpectEvents({{Type::Enter}});
        ExpectCurrentOccupancy(false);
    }
};

TEST_P(InvalidColliderWorldTests, InvalidDimensionEndsPairOnceAndCanRecover) {
    auto invalid = *registry.GetComponent<Collider>(second);

    for (const auto dimension : colliderDimensions) {
        if (UsesDimension(GetParam(), dimension)) {
            DimensionValue(invalid, dimension) = 0.0f;
            break;
        }
    }

    ExpectInvalidColliderEndsPair(invalid);
}

TEST_P(InvalidColliderWorldTests, InvalidOffsetEndsPairOnceAndCanRecover) {
    auto invalid = *registry.GetComponent<Collider>(second);
    invalid.offset.x = std::numeric_limits<float>::quiet_NaN();
    ExpectInvalidColliderEndsPair(invalid);
}

TEST_P(InvalidColliderWorldTests, InvalidLayerEndsPairOnceAndCanRecover) {
    auto invalid = *registry.GetComponent<Collider>(second);
    invalid.layer = 32;
    ExpectInvalidColliderEndsPair(invalid);
}

INSTANTIATE_TEST_SUITE_P(AllShapes, InvalidColliderWorldTests, testing::ValuesIn(colliderTestShapes), [](const testing::TestParamInfo<Shape> &info) { return std::string{ShapeName(info.param)}; });
