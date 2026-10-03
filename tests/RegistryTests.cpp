#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/Types.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <vector>
    // mock components for testing

namespace {
    struct ComponentA {
        int value = 0;
    };

    struct ComponentB {};
} // namespace

class RegistryTests : public testing::Test {
protected:
    ECS::Registry registry;

    ECS::EntityID CreateWithA(int value) {
        const auto entity = registry.CreateEntity();
        registry.AddComponent<ComponentA>(entity).value = value;
        return entity;
    }

    ECS::EntityID CreateWithBoth(int value) {
        const auto entity = CreateWithA(value);
        registry.AddComponent<ComponentB>(entity);
        return entity;
    }

    void ExpectValue(ECS::EntityID entity, int expected) {
        const auto *component = registry.GetComponent<ComponentA>(entity);

        ASSERT_NE(component, nullptr);
        EXPECT_EQ(component->value, expected);
    }

    std::vector<ECS::EntityID> VisitMatching() {
        std::vector<ECS::EntityID> visited;

        registry.ForEach<ComponentA, ComponentB>([&](ECS::Entity entity, const ComponentA *, const ComponentB *) { visited.push_back(static_cast<ECS::EntityID>(entity)); });

        std::ranges::sort(visited);
        return visited;
    }

    ECS::EntityID Recycle(ECS::EntityID entity) {
        // Unused slots come before recycled slots in the registry's queue.
        // Fill them first so the next creation must reuse this slot.
        while (registry.GetLivingEntities().size() < ECS::MAX_ENTITIES - 1) {
            registry.CreateEntity();
        }

        registry.DestroyEntity(entity);
        return registry.CreateEntity();
    }
};

TEST_F(RegistryTests, CreateEntitiesDistinct) {
    const auto first = registry.CreateEntity();
    const auto second = registry.CreateEntity();

    EXPECT_TRUE(registry.IsValid(first));
    EXPECT_TRUE(registry.IsValid(second));
    EXPECT_NE(first, second);
}

TEST_F(RegistryTests, DestroyEntityInvalidatesOwnHandle) {
    const auto first = registry.CreateEntity();
    const auto second = registry.CreateEntity();

    registry.DestroyEntity(first);

    EXPECT_FALSE(registry.IsValid(first));
    EXPECT_TRUE(registry.IsValid(second));
}

TEST_F(RegistryTests, RemoveComponentPreservesOtherData) {
    const auto first = CreateWithA(10);
    const auto middle = CreateWithA(20);
    const auto last = CreateWithA(30);

    registry.RemoveComponent<ComponentA>(middle);

    EXPECT_FALSE(registry.HasComponent<ComponentA>(middle));
    EXPECT_EQ(registry.GetComponent<ComponentA>(middle), nullptr);

    ExpectValue(first, 10);
    ExpectValue(last, 30);
}

TEST_F(RegistryTests, ForEachVisitsOnlyEntitiesSpecified) {
    CreateWithA(10);

    const auto onlyB = registry.CreateEntity();
    registry.AddComponent<ComponentB>(onlyB);

    const auto both = CreateWithBoth(20);

    const std::vector<ECS::EntityID> expected{both};
    EXPECT_EQ(VisitMatching(), expected);
}

TEST_F(RegistryTests, RecycledSlotReceivesNewHandleVersion) {
    const auto original = registry.CreateEntity();
    const auto replacement = Recycle(original);

    ASSERT_EQ(ECS::GetEntityIndex(original), ECS::GetEntityIndex(replacement));

    EXPECT_NE(original, replacement);
    EXPECT_EQ(ECS::GetEntityVersion(replacement), ECS::GetEntityVersion(original) + 1);

    EXPECT_FALSE(registry.IsValid(original));
    EXPECT_TRUE(registry.IsValid(replacement));
}

TEST_F(RegistryTests, StaleHandleCannotAccessReplacementComponents) {
    const auto original = CreateWithBoth(10);
    const auto replacement = Recycle(original);

    ASSERT_EQ(ECS::GetEntityIndex(original), ECS::GetEntityIndex(replacement));

    // Components from the destroyed entity must not survive recycling.
    EXPECT_FALSE(registry.HasComponent<ComponentA>(replacement));
    EXPECT_FALSE(registry.HasComponent<ComponentB>(replacement));

    registry.AddComponent<ComponentA>(replacement).value = 42;
    registry.AddComponent<ComponentB>(replacement);

    EXPECT_EQ(registry.GetComponent<ComponentA>(original), nullptr);
    EXPECT_EQ(registry.GetComponent<ComponentB>(original), nullptr);

    EXPECT_FALSE(registry.HasComponent<ComponentA>(original));
    EXPECT_FALSE(registry.HasComponent<ComponentB>(original));

    EXPECT_TRUE(registry.HasComponent<ComponentB>(replacement));
    ExpectValue(replacement, 42);
}

TEST_F(RegistryTests, StaleHandleCannotRemoveReplacementComponent) {
    const auto original = CreateWithA(10);
    const auto replacement = Recycle(original);

    ASSERT_EQ(ECS::GetEntityIndex(original), ECS::GetEntityIndex(replacement));

    registry.AddComponent<ComponentA>(replacement).value = 42;

    registry.RemoveComponent<ComponentA>(original);

    EXPECT_TRUE(registry.IsValid(replacement));
    EXPECT_TRUE(registry.HasComponent<ComponentA>(replacement));
    ExpectValue(replacement, 42);
}

TEST_F(RegistryTests, DestroyEntityRemovesAllComponents) {
    const auto destroyed = CreateWithBoth(10);
    const auto survivor = CreateWithBoth(20);

    registry.DestroyEntity(destroyed);

    EXPECT_FALSE(registry.GetPool<ComponentA>()->Has(destroyed));
    EXPECT_FALSE(registry.GetPool<ComponentB>()->Has(destroyed));

    EXPECT_TRUE(registry.HasComponent<ComponentB>(survivor));
    ExpectValue(survivor, 20);

    const std::vector<ECS::EntityID> expected{survivor};
    EXPECT_EQ(VisitMatching(), expected);
    EXPECT_EQ(registry.GetLivingEntities(), expected);
}

TEST_F(RegistryTests, DestroyingAlreadyDestroyedEntityChangesNothing) {
    const auto destroyed = CreateWithBoth(10);
    const auto survivor = CreateWithBoth(20);

    registry.DestroyEntity(destroyed);

    const auto livingBefore = registry.GetLivingEntities();
    const auto matchingBefore = VisitMatching();

    registry.DestroyEntity(destroyed);

    EXPECT_EQ(registry.GetLivingEntities(), livingBefore);
    EXPECT_EQ(VisitMatching(), matchingBefore);
    EXPECT_FALSE(registry.IsValid(destroyed));
    EXPECT_TRUE(registry.IsValid(survivor));

    ExpectValue(survivor, 20);
}

TEST_F(RegistryTests, RemovingAbsentComponentPreservesOtherComponents) {
    const auto withoutB = CreateWithA(10);
    const auto withB = CreateWithBoth(20);

    registry.RemoveComponent<ComponentB>(withoutB);

    EXPECT_FALSE(registry.HasComponent<ComponentB>(withoutB));
    EXPECT_TRUE(registry.HasComponent<ComponentB>(withB));

    ExpectValue(withoutB, 10);
    ExpectValue(withB, 20);

    const std::vector<ECS::EntityID> expected{withB};
    EXPECT_EQ(VisitMatching(), expected);
}

TEST_F(RegistryTests, ForEachHandlesEmptyPools) {
    EXPECT_TRUE(VisitMatching().empty());

    CreateWithA(10);
    EXPECT_TRUE(VisitMatching().empty());

    const auto onlyB = registry.CreateEntity();
    registry.AddComponent<ComponentB>(onlyB);

    EXPECT_TRUE(VisitMatching().empty());
}

TEST_F(RegistryTests, ForEachVisitsEveryMatchingEntityOnce) {
    const auto first = CreateWithBoth(10);
    CreateWithA(20);
    const auto second = CreateWithBoth(30);

    const auto onlyB = registry.CreateEntity();
    registry.AddComponent<ComponentB>(onlyB);

    std::vector<ECS::EntityID> expected{first, second};
    std::ranges::sort(expected);

    EXPECT_EQ(VisitMatching(), expected);
}