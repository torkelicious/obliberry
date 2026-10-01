#include "Core/ResourceManager.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/Types.h"
#include "IO/Loaders/EntityFactory.h"
#include "TestingUtils.h"
#include "nlohmann/json_fwd.hpp"
#include <gtest/gtest.h>


class EntitySerializationTests : public testing::Test {
protected:
    ECS::Registry sourceRegistry;
    ECS::Registry destinationRegistry;

    Core::ResourceManager &resources = Core::ResourceManager::GetInstance();

    void SetUp() override {
        IO::EntityFactory::RegisterSerializers();
        IO::EntityFactory::RegisterDeserializers();
    }

    ECS::Entity CreateEntity(ECS::Registry &registry) { return ECS::Entity{registry.CreateEntity(), &registry}; }

    nlohmann::json Serialize(ECS::Entity &entity) {
        nlohmann::json data;
        IO::EntityFactory::SerializeEntity(entity, data, resources);
        return data;
    }

    std::string GetUUID(const ECS::Entity &entity) { return entity.GetRegistry()->GetEntityUUID(static_cast<ECS::EntityID>(entity)); }
};


// Components should survive serialization and deserialization
// (there are some exceptions, but these components are expected to survive)
TEST_F(EntitySerializationTests, ComponentRoundtrip) {
    const std::string expectedName = "TestEntity";
    constexpr glm::vec3 expectedPosition{2.0f, 3.0f, 4.0f};
    constexpr glm::vec3 expectedRotation{0.25f, 0.5f, 0.75f};
    constexpr glm::vec3 expectedScale{2.0f, 3.0f, 4.0f};
    constexpr float expectedTimePerStep = 0.75f;

    constexpr ECS::Components::ColliderComponent expectedCollider{
            .shape = ECS::Components::ColliderShape::Cylinder,
            .orientation = ECS::Components::ColliderOrientation::Billboard,
            .offset = {0.25f, 0.5f, 0.75f},
            .size = {2.0f, 3.0f, 4.0f},
            .radius = 0.75f,
            .height = 2.0f,
            .isTrigger = true,
            .layer = 3,
            .mask = (1u << 1) | (1u << 31),
    };


    auto source = CreateEntity(sourceRegistry);
    source.SetName(expectedName);


    auto &transform = source.AddComponent<ECS::Components::TransformComponent>();

    transform.transform.SetPosition(expectedPosition);
    transform.transform.SetRotation(expectedRotation);
    transform.transform.SetScale(expectedScale);

    auto &movement = source.AddComponent<ECS::Components::MovementComponent>();
    movement.timePerStep = expectedTimePerStep;
    movement.autoMove = true;

    source.AddComponent<ECS::Components::ColliderComponent>(expectedCollider);

    const auto data = nlohmann::json::parse(Serialize(source).dump());
    auto loaded = CreateEntity(destinationRegistry);
    IO::EntityFactory::DeserializeEntity(loaded, data, resources);

    const auto *loadedTransform = loaded.GetComponent<ECS::Components::TransformComponent>();
    const auto *loadedMovement = loaded.GetComponent<ECS::Components::MovementComponent>();
    const auto *loadedCollider = loaded.GetComponent<ECS::Components::ColliderComponent>();

    ASSERT_NE(loadedTransform, nullptr);
    ASSERT_NE(loadedMovement, nullptr);
    ASSERT_NE(loadedCollider, nullptr);

    EXPECT_EQ(loaded.GetName(), expectedName);

    TestUtils::GLM_VecExpectFloat(loadedTransform->transform.GetPosition(), expectedPosition);
    TestUtils::GLM_VecExpectFloat(loadedTransform->transform.GetRotation(), expectedRotation);
    TestUtils::GLM_VecExpectFloat(loadedTransform->transform.GetScale(), expectedScale);

    EXPECT_FLOAT_EQ(loadedMovement->timePerStep, expectedTimePerStep);
    EXPECT_TRUE(loadedMovement->autoMove);

    EXPECT_EQ(*loadedCollider, expectedCollider);
}

// preserved uuids should be restored
TEST_F(EntitySerializationTests, PreserveUUIDRestoresSavedIdentity) {
    auto source = CreateEntity(sourceRegistry);
    const std::string savedUUID = GetUUID(source);
    const auto data = Serialize(source);

    auto loaded = CreateEntity(destinationRegistry);

    IO::EntityFactory::DeserializeEntity(loaded, data, resources, true);

    EXPECT_EQ(GetUUID(loaded), savedUUID);
}

// non-preserved should not keep uuid
TEST_F(EntitySerializationTests, WithoutPreserveKeepsNewIdentity) {
    auto source = CreateEntity(sourceRegistry);
    const auto data = Serialize(source);

    auto loaded = CreateEntity(destinationRegistry);
    const std::string generatedUUID = GetUUID(loaded);

    ASSERT_NE(generatedUUID, GetUUID(source));

    IO::EntityFactory::DeserializeEntity(loaded, data, resources, false);

    EXPECT_EQ(GetUUID(loaded), generatedUUID);
    EXPECT_NE(GetUUID(loaded), GetUUID(source));
}

// a fucked up collider should not break the transform or loading.
TEST_F(EntitySerializationTests, MalformedColliderKeepsTransformValid) {
    const auto data = nlohmann::json::parse(
            R"(
        {
            "components": {
                "TransformComponent": {
                    "position": [1, 2, 3]
                },
                "ColliderComponent": {
                    "version": 2,
                    "shape": "Box",
                    "size": [0, 1, 1]
                }
            }
        }
        )");

    auto loaded = CreateEntity(destinationRegistry);

    ASSERT_NO_THROW(IO::EntityFactory::DeserializeEntity(loaded, data, resources, true));
    EXPECT_TRUE(destinationRegistry.IsValid(static_cast<ECS::EntityID>(loaded)));
    EXPECT_FALSE(loaded.HasComponent<ECS::Components::ColliderComponent>());

    const auto *transform = loaded.GetComponent<ECS::Components::TransformComponent>();
    ASSERT_NE(transform, nullptr);
    TestUtils::GLM_VecExpectFloat(transform->transform.GetPosition(), glm::vec3{1.0f, 2.0f, 3.0f});
}

TEST_F(EntitySerializationTests, DuplicateUUIDKeepsGeneratedIdentity) {
    auto source = CreateEntity(sourceRegistry);
    const auto data = Serialize(source);
    const std::string savedUUID = GetUUID(source);

    auto first = CreateEntity(destinationRegistry);

    IO::EntityFactory::DeserializeEntity(first, data, resources, true);

    ASSERT_EQ(GetUUID(first), savedUUID);

    auto second = CreateEntity(destinationRegistry);
    const std::string secondOriginalUUID = GetUUID(second);

    IO::EntityFactory::DeserializeEntity(second, data, resources, true);

    EXPECT_TRUE(destinationRegistry.IsValid(static_cast<ECS::EntityID>(first)));
    EXPECT_TRUE(destinationRegistry.IsValid(static_cast<ECS::EntityID>(second)));

    EXPECT_EQ(GetUUID(first), savedUUID);
    EXPECT_EQ(GetUUID(second), secondOriginalUUID);
    EXPECT_NE(GetUUID(first), GetUUID(second));
}
