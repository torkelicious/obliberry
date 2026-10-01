#include <gtest/gtest.h>

#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/Types.h"
#include <string>

// new entities should get unique uuids
TEST(EntityUUIDTests, NewEntityDistinctNonEmptyUUID) {
    ECS::Registry registry;

    const ECS::EntityID first = registry.CreateEntity();
    const ECS::EntityID second = registry.CreateEntity();

    const std::string firstUUID = registry.GetEntityUUID(first);
    const std::string secondUUID = registry.GetEntityUUID(second);

    EXPECT_FALSE(firstUUID.empty());
    EXPECT_FALSE(secondUUID.empty());
    EXPECT_NE(firstUUID, secondUUID);
}

// an already existing uuid should not be allowed to be assigned to another
TEST(EntityUUIDTests, DuplicateAssignmentPreservesOriginalUUID) {
    ECS::Registry registry;

    const ECS::EntityID first = registry.CreateEntity();
    const ECS::EntityID second = registry.CreateEntity();

    const std::string firstUUID = registry.GetEntityUUID(first);
    const std::string secondUUID = registry.GetEntityUUID(second);

    const bool accepted = registry.SetEntityUUID(second, firstUUID);
    EXPECT_FALSE(accepted);
    EXPECT_EQ(registry.GetEntityUUID(first), firstUUID);
    EXPECT_EQ(registry.GetEntityUUID(second), secondUUID);
}

// empty uuids assignment should be rejected
TEST(EntityUUIDTests, EmptyAssignmentPreservesUUID) {
    ECS::Registry registry;

    const ECS::EntityID entity = registry.CreateEntity();
    const std::string originalUUID = registry.GetEntityUUID(entity);
    const bool accepted = registry.SetEntityUUID(entity, "");

    EXPECT_FALSE(accepted);
    EXPECT_EQ(registry.GetEntityUUID(entity), originalUUID);
}

// this shud work lol
TEST(EntityUUIDTests, AssigningOwnUUIDSucceeds) {
    ECS::Registry registry;

    const ECS::EntityID entity = registry.CreateEntity();
    const std::string originalUUID = registry.GetEntityUUID(entity);

    const bool accepted = registry.SetEntityUUID(entity, originalUUID);

    EXPECT_TRUE(accepted);
    EXPECT_EQ(registry.GetEntityUUID(entity), originalUUID);
}

// We should not be able to assign to deleted entities, ..duh
TEST(EntityUUIDTests, DestroyedHandleRejectsAssignment) {
    ECS::Registry registry;

    const ECS::EntityID entity = registry.CreateEntity();
    const std::string originalUUID = registry.GetEntityUUID(entity);

    registry.DestroyEntity(entity);

    ASSERT_FALSE(registry.IsValid(entity));
    EXPECT_FALSE(registry.SetEntityUUID(entity, originalUUID));
}
