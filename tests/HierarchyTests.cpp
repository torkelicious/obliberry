#include <gtest/gtest.h>

#include "ECS/Components/RelationshipComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/Systems/HierarchySystem.h"
#include "ECS/Types.h"
#include "TestingUtils.h"

#include <algorithm>

class HierarchyTests : public testing::Test {
protected:
    ECS::Registry registry;

    ECS::EntityID CreateEntity(const glm::vec3 &position = glm::vec3{0.0f}) {
        const auto id = registry.CreateEntity();
        registry.AddComponent<ECS::Components::TransformComponent>(id).transform.SetPosition(position);
        registry.AddComponent<ECS::Components::RelationshipComponent>(id);
        UpdateHierarchy();
        return id;
    }

    void UpdateHierarchy() { ECS::Systems::HierarchySystem::Propagate(registry); }

    void ExpectParent(ECS::EntityID child, ECS::EntityID expected) {
        const auto *relationship = registry.GetComponent<ECS::Components::RelationshipComponent>(child);

        ASSERT_NE(relationship, nullptr);
        EXPECT_EQ(relationship->parent, expected);
    }

    void ExpectChild(ECS::EntityID parent, ECS::EntityID child, bool expected = true) {
        const auto *relationship = registry.GetComponent<ECS::Components::RelationshipComponent>(parent);

        ASSERT_NE(relationship, nullptr);

        const auto count = std::count(relationship->children.begin(), relationship->children.end(), child);

        EXPECT_EQ(count, expected ? 1 : 0);
    }

    void ExpectWorldPosition(ECS::EntityID entity, const glm::vec3 &expected) {
        const auto *transform = registry.GetComponent<ECS::Components::TransformComponent>(entity);
        ASSERT_NE(transform, nullptr);
        TestUtils::GLM_VecExpectFloat(transform->worldTransform.GetPosition(), expected);
    }
};

// check that reparent updates both parent and child
TEST_F(HierarchyTests, ReparentUpdatesBoth) {
    const auto firstParent = CreateEntity();
    const auto secondParent = CreateEntity();
    const auto child = CreateEntity();

    registry.Reparent(child, firstParent);

    ExpectParent(child, firstParent);
    ExpectChild(firstParent, child);

    registry.Reparent(child, secondParent);

    ExpectParent(child, secondParent);
    ExpectChild(firstParent, child, false);
    ExpectChild(secondParent, child);
}

// Parenting preserves world position
TEST_F(HierarchyTests, ParentMovementPropagatesToChild) {
    const auto parent = CreateEntity({10.0f, 0.0f, 0.0f});
    const auto child = CreateEntity({12.0f, 0.0f, 0.0f});

    registry.Reparent(child, parent);
    UpdateHierarchy();

    ExpectWorldPosition(child, {12.0f, 0.0f, 0.0f});

    auto *transform = registry.GetComponent<ECS::Components::TransformComponent>(parent);

    ASSERT_NE(transform, nullptr);
    transform->transform.SetPosition({20.0f, 0.0f, 0.0f});

    UpdateHierarchy();

    ExpectWorldPosition(child, {22.0f, 0.0f, 0.0f});
}

// Detaching must preserve world position and remove the link
TEST_F(HierarchyTests, UnparentPreservesWorldPosition) {
    const auto parent = CreateEntity({10.0f, 0.0f, 0.0f});
    const auto child = CreateEntity({12.0f, 0.0f, 0.0f});

    registry.Reparent(child, parent);
    UpdateHierarchy();

    registry.Reparent(child, ECS::INVALID_ENTITY_ID);
    UpdateHierarchy();

    ExpectParent(child, ECS::INVALID_ENTITY_ID);
    ExpectChild(parent, child, false);
    ExpectWorldPosition(child, {12.0f, 0.0f, 0.0f});
}

// Making a descendant the parent parent would create a cycle
TEST_F(HierarchyTests, DescendantCannotBecomeParent) {
    const auto root = CreateEntity();
    const auto child = CreateEntity();
    const auto grandchild = CreateEntity();

    registry.Reparent(child, root);
    registry.Reparent(grandchild, child);

    registry.Reparent(root, grandchild);

    ExpectParent(root, ECS::INVALID_ENTITY_ID);
    ExpectParent(child, root);
    ExpectParent(grandchild, child);

    ExpectChild(root, child);
    ExpectChild(child, grandchild);
    ExpectChild(grandchild, root, false);
}

TEST_F(HierarchyTests, DestroyParentDestroysDescendants) {
    const auto parent = CreateEntity();
    const auto child = CreateEntity();
    const auto grandchild = CreateEntity();
    const auto unrelated = CreateEntity();

    registry.Reparent(child, parent);
    registry.Reparent(grandchild, child);

    registry.DestroyEntity(parent);

    EXPECT_FALSE(registry.IsValid(parent));
    EXPECT_FALSE(registry.IsValid(child));
    EXPECT_FALSE(registry.IsValid(grandchild));
    EXPECT_TRUE(registry.IsValid(unrelated));
}
