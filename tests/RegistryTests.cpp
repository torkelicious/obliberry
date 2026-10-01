#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/Types.h"
#include <gtest/gtest.h>
#include <vector>
namespace {
    // mock components for testing

    struct ComponentA {
        int value = 0;
    };

    struct ComponentB {};
} // namespace


// two newly created entities should never be the same.
TEST(RegistryTests, CreateEntitiesDistinct) {
    ECS::Registry registry;

    const ECS::EntityID first = registry.CreateEntity();
    const ECS::EntityID second = registry.CreateEntity();

    EXPECT_TRUE(registry.IsValid(first));
    EXPECT_TRUE(registry.IsValid(second));
    EXPECT_NE(first, second);
}


// Make sure destroying an entity correctly invalidates its Own entity handle, but does not affect others!
TEST(RegistryTests, DestroyEntityInvalidatesOwnHandle) {
    ECS::Registry registry;

    const ECS::EntityID first = registry.CreateEntity();
    const ECS::EntityID second = registry.CreateEntity();

    registry.DestroyEntity(first);

    EXPECT_FALSE(registry.IsValid(first));
    EXPECT_TRUE(registry.IsValid(second));
}

// Should not corrupt other entities or data on Remove-Component operations.
TEST(RegistryTests, RemoveComponentPreservesOtherData) {
    ECS::Registry registry;

    const ECS::EntityID first = registry.CreateEntity();
    const ECS::EntityID middle = registry.CreateEntity();
    const ECS::EntityID last = registry.CreateEntity();

    registry.AddComponent<ComponentA>(first).value = 10;
    registry.AddComponent<ComponentA>(middle).value = 20;
    registry.AddComponent<ComponentA>(last).value = 30;

    registry.RemoveComponent<ComponentA>(middle);

    EXPECT_FALSE(registry.HasComponent<ComponentA>(middle));
    EXPECT_EQ(registry.GetComponent<ComponentA>(middle), nullptr);

    const auto *firstComponent = registry.GetComponent<ComponentA>(first);
    const auto *lastComponent = registry.GetComponent<ComponentA>(last);

    ASSERT_NE(firstComponent, nullptr);
    ASSERT_NE(lastComponent, nullptr);

    EXPECT_EQ(firstComponent->value, 10);
    EXPECT_EQ(lastComponent->value, 30);
}

// ForEach operations should ONLY operate on the specified components (it shall only visit ents with both comps!!!)
TEST(RegistryTests, ForEachVisitsOnlyEntitiesSpecified) {
    ECS::Registry registry;

    const ECS::EntityID onlyA = registry.CreateEntity();
    const ECS::EntityID onlyB = registry.CreateEntity();
    const ECS::EntityID both = registry.CreateEntity();

    registry.AddComponent<ComponentA>(onlyA);
    registry.AddComponent<ComponentB>(onlyB);
    registry.AddComponent<ComponentA>(both);
    registry.AddComponent<ComponentB>(both);

    std::vector<ECS::EntityID> visited;

    registry.ForEach<ComponentA, ComponentB>([&](ECS::Entity entity, const ComponentA *, const ComponentB *) { visited.push_back(static_cast<ECS::EntityID>(entity)); });

    ASSERT_EQ(visited.size(), 1u);
    EXPECT_EQ(visited.front(), both);
}
