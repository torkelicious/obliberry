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

TEST(EntityUUIDTests, UniqueAssignmentUpdatesOnlySelectedEntity) {
    ECS::Registry registry;

    const auto first = registry.CreateEntity();
    const auto second = registry.CreateEntity();

    const std::string firstOriginalUUID = registry.GetEntityUUID(first);
    const std::string secondOriginalUUID = registry.GetEntityUUID(second);
    const std::string assignedUUID = Core::Utils::UUID::UUIDGenerator::Generate();

    ASSERT_NE(assignedUUID, firstOriginalUUID);
    ASSERT_NE(assignedUUID, secondOriginalUUID);

    ASSERT_TRUE(registry.SetEntityUUID(first, assignedUUID));

    EXPECT_EQ(registry.GetEntityUUID(first), assignedUUID);
    EXPECT_EQ(registry.GetEntityUUID(second), secondOriginalUUID);
    EXPECT_TRUE(registry.IsValid(first));
    EXPECT_TRUE(registry.IsValid(second));
}

// reused slot should have a unique new id
TEST(EntityUUIDTests, RecycledSlotGetsNewUUID) {
    ECS::Registry registry;

    const auto original = registry.CreateEntity();
    const std::string originalUUID = registry.GetEntityUUID(original);

    while (registry.GetLivingEntities().size() < ECS::MAX_ENTITIES - 1) {
        registry.CreateEntity();
    }

    registry.DestroyEntity(original);

    registry.DestroyEntity(original);
    const auto replacement = registry.CreateEntity();

    ASSERT_EQ(ECS::GetEntityIndex(original), ECS::GetEntityIndex(replacement));

    const std::string replacementUUID = registry.GetEntityUUID(replacement);

    EXPECT_FALSE(replacementUUID.empty());
    EXPECT_NE(replacementUUID, originalUUID);

    EXPECT_TRUE(registry.GetEntityUUID(original).empty());
}

// generated uuids should use expected format
TEST(EntityUUIDTests, GeneratedUUIDHasVersion4Format) {
    const std::string uuid = Core::Utils::UUID::UUIDGenerator::Generate();

    ASSERT_EQ(uuid.size(), 36u);

    static const std::string_view hexDigits = "0123456789abcdef";
    static const std::string_view variantDigits = "89ab";

    for (std::size_t i = 0; i < uuid.size(); ++i) {
        SCOPED_TRACE(i);
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            EXPECT_EQ(uuid[i], '-');
        } else {
            EXPECT_NE(hexDigits.find(uuid[i]), std::string_view::npos);
        }
    }

    EXPECT_EQ(uuid[14], '4');
    EXPECT_NE(variantDigits.find(uuid[19]), std::string_view::npos);
}
