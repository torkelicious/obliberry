#include "Core/EngineContext.h"
#include "Core/ResourceManager.h"
#include "Core/Utils/UUID.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/PersistentTagComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/Types.h"
#include "IO/Loaders/EntityFactory.h"
#include "IO/SceneSerialization.h"
#include "IO/VFS/VFS.h"
#include "Scenes/Scene.h"
#include "Scenes/SceneManager.h"
#include "TestingUtils.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

class SceneTests : public testing::Test {
protected:
    Core::EngineContext context;
    Scenes::SceneManager manager;

    std::filesystem::path directory;
    bool ownsDirectory = false;

    void SetUp() override {
        context.resources = &Core::ResourceManager::GetInstance();
        context.sceneManager = &manager;
        context.isEditorMode = true;

        manager.SetContext(context);

        IO::EntityFactory::RegisterSerializers();
        IO::EntityFactory::RegisterDeserializers();

        directory = std::filesystem::temp_directory_path() / ("obliberry-scene-test-" + Core::Utils::UUID::UUIDGenerator::Generate());

        ASSERT_TRUE(std::filesystem::create_directory(directory));
        ownsDirectory = true;

        context.ProjectRootPath = directory;

        IO::VFS::MountProject(directory / "project.json");

        ASSERT_NO_FATAL_FAILURE(WriteScene("a.json"));
        ASSERT_NO_FATAL_FAILURE(WriteScene("b.json"));
        ASSERT_NO_FATAL_FAILURE(WriteScene("c.json"));
    }

    void TearDown() override {
        manager.ClearCurrentScene();
        IO::VFS::UnmountProject();

        if (ownsDirectory) {
            std::error_code error;
            std::filesystem::remove_all(directory, error);
            EXPECT_FALSE(error) << error.message();
        }
    }

    void WriteScene(const std::string &filename, const nlohmann::json &entities = nlohmann::json::array()) {
        const nlohmann::json document{{"properties", {{"name", filename}, {"lighting", false}}}, {"entities", entities}, {"PostProcessing", nlohmann::json::array()}};

        std::ofstream file(directory / filename);
        ASSERT_TRUE(file.is_open());

        file << document.dump();
        file.close();

        ASSERT_FALSE(file.fail());
    }

    void SwitchTo(const std::string &filename) {
        manager.LoadSceneByPath(filename);

        ASSERT_NE(manager.GetCurrentScene(), nullptr);
        EXPECT_EQ(manager.GetCurrentScenePath(), filename);
    }

    ECS::Registry &Registry() { return manager.GetCurrentScene()->GetRegistry(); }

    ECS::Entity CreateEntity(const std::string &name, const bool persistent) {
        auto &registry = Registry();
        ECS::Entity entity{registry.CreateEntity(), &registry};

        entity.SetName(name);
        entity.AddComponent<ECS::Components::TransformComponent>();

        if (persistent) {
            entity.AddComponent<ECS::Components::PersistentTagComponent>();
        }

        return entity;
    }

    std::string UUID(const ECS::Entity &entity) { return entity.GetRegistry()->GetEntityUUID(static_cast<ECS::EntityID>(entity)); }

    ECS::EntityID FindUUID(const std::string &uuid) {
        auto &registry = Registry();

        for (const auto id : registry.GetLivingEntities()) {
            if (registry.IsValid(id) && registry.GetEntityUUID(id) == uuid) {
                return id;
            }
        }

        return ECS::INVALID_ENTITY_ID;
    }

    std::size_t CountUUID(const std::string &uuid) {
        std::size_t count = 0;
        auto &registry = Registry();

        for (const auto id : registry.GetLivingEntities()) {
            if (registry.IsValid(id) && registry.GetEntityUUID(id) == uuid) {
                ++count;
            }
        }

        return count;
    }

    nlohmann::json Serialize(ECS::Entity &entity) {
        nlohmann::json data;

        IO::EntityFactory::SerializeEntity(entity, data, *context.resources);

        return data;
    }
};

TEST_F(SceneTests, PersistentEntityRetainsIdentityAndComponentState) {
    ASSERT_NO_FATAL_FAILURE(SwitchTo("a.json"));

    auto entity = CreateEntity("Persistent Entity", true);
    const auto uuid = UUID(entity);

    auto *transform = entity.GetComponent<ECS::Components::TransformComponent>();

    ASSERT_NE(transform, nullptr);

    const glm::vec3 position{2.0f, 3.0f, 4.0f};
    const glm::vec3 rotation{0.25f, 0.5f, 0.75f};
    const glm::vec3 scale{2.0f, 3.0f, 4.0f};

    transform->transform.SetPosition(position);
    transform->transform.SetRotation(rotation);
    transform->transform.SetScale(scale);

    auto &movement = entity.AddComponent<ECS::Components::MovementComponent>();

    movement.timePerStep = 0.75f;
    movement.autoMove = true;

    auto &collider = entity.AddComponent<ECS::Components::ColliderComponent>();

    collider.shape = ECS::Components::ColliderShape::Cylinder;
    collider.radius = 0.75f;
    collider.height = 2.0f;
    collider.layer = 31;
    collider.mask = 1u;
    collider.isTrigger = true;

    const auto expectedCollider = collider;

    const auto temporaryUUID = UUID(CreateEntity("Temporary", false));

    for (const char *destination : {"b.json", "c.json"}) {
        SCOPED_TRACE(destination);

        ASSERT_NO_FATAL_FAILURE(SwitchTo(destination));

        const auto id = FindUUID(uuid);
        ASSERT_NE(id, ECS::INVALID_ENTITY_ID);
        EXPECT_EQ(CountUUID(uuid), 1u);

        ECS::Entity loaded{id, &Registry()};

        EXPECT_EQ(loaded.GetName(), "Persistent Entity");
        EXPECT_TRUE(loaded.HasComponent<ECS::Components::PersistentTagComponent>());

        const auto *actualTransform = loaded.GetComponent<ECS::Components::TransformComponent>();
        const auto *actualMovement = loaded.GetComponent<ECS::Components::MovementComponent>();
        const auto *actualCollider = loaded.GetComponent<ECS::Components::ColliderComponent>();

        ASSERT_NE(actualTransform, nullptr);
        ASSERT_NE(actualMovement, nullptr);
        ASSERT_NE(actualCollider, nullptr);

        TestUtils::GLM_VecExpectFloat(actualTransform->transform.GetPosition(), position);
        TestUtils::GLM_VecExpectFloat(actualTransform->transform.GetRotation(), rotation);
        TestUtils::GLM_VecExpectFloat(actualTransform->transform.GetScale(), scale);

        EXPECT_FLOAT_EQ(actualMovement->timePerStep, 0.75f);
        EXPECT_TRUE(actualMovement->autoMove);
        EXPECT_EQ(*actualCollider, expectedCollider);

        EXPECT_EQ(FindUUID(temporaryUUID), ECS::INVALID_ENTITY_ID);
    }
}

TEST_F(SceneTests, MatchingUUIDReplacesDestinationCopyWithLiveState) {
    ASSERT_NO_FATAL_FAILURE(SwitchTo("a.json"));

    auto entity = CreateEntity("Original Name", true);
    const auto uuid = UUID(entity);

    const auto oldCopy = Serialize(entity);

    ASSERT_NO_FATAL_FAILURE(WriteScene("b.json", nlohmann::json::array({oldCopy})));

    entity.SetName("Live Name");

    auto *transform = entity.GetComponent<ECS::Components::TransformComponent>();

    ASSERT_NE(transform, nullptr);
    transform->transform.SetPosition({8.0f, 9.0f, 10.0f});

    ASSERT_NO_FATAL_FAILURE(SwitchTo("b.json"));

    EXPECT_EQ(CountUUID(uuid), 1u);

    const auto id = FindUUID(uuid);
    ASSERT_NE(id, ECS::INVALID_ENTITY_ID);

    ECS::Entity loaded{id, &Registry()};

    EXPECT_EQ(loaded.GetName(), "Live Name");

    const auto *actual = loaded.GetComponent<ECS::Components::TransformComponent>();

    ASSERT_NE(actual, nullptr);

    TestUtils::GLM_VecExpectFloat(actual->transform.GetPosition(), glm::vec3{8.0f, 9.0f, 10.0f});
}

TEST_F(SceneTests, SameNameWithDifferentUUIDsRemainsDistinct) {
    ASSERT_NO_FATAL_FAILURE(SwitchTo("a.json"));

    auto persistent = CreateEntity("Shared Name", true);
    const auto persistentUUID = UUID(persistent);

    auto destinationCopy = Serialize(persistent);
    const auto destinationUUID = Core::Utils::UUID::UUIDGenerator::Generate();

    ASSERT_NE(destinationUUID, persistentUUID);

    destinationCopy["uuid"] = destinationUUID;
    destinationCopy["components"]["TransformComponent"]["position"] = {20.0f, 0.0f, 0.0f};

    ASSERT_NO_FATAL_FAILURE(WriteScene("b.json", nlohmann::json::array({destinationCopy})));

    ASSERT_NO_FATAL_FAILURE(SwitchTo("b.json"));

    EXPECT_EQ(CountUUID(persistentUUID), 1u);
    EXPECT_EQ(CountUUID(destinationUUID), 1u);

    const auto firstID = FindUUID(persistentUUID);
    const auto secondID = FindUUID(destinationUUID);

    ASSERT_NE(firstID, ECS::INVALID_ENTITY_ID);
    ASSERT_NE(secondID, ECS::INVALID_ENTITY_ID);
    EXPECT_NE(firstID, secondID);

    ECS::Entity first{firstID, &Registry()};
    ECS::Entity second{secondID, &Registry()};

    EXPECT_EQ(first.GetName(), "Shared Name");
    EXPECT_EQ(second.GetName(), "Shared Name");

    const auto *secondTransform = second.GetComponent<ECS::Components::TransformComponent>();

    ASSERT_NE(secondTransform, nullptr);

    TestUtils::GLM_VecExpectFloat(secondTransform->transform.GetPosition(), glm::vec3{20.0f, 0.0f, 0.0f});
}

TEST_F(SceneTests, RenamingDoesNotChangePersistenceIdentity) {
    ASSERT_NO_FATAL_FAILURE(SwitchTo("a.json"));

    auto entity = CreateEntity("Before", true);
    const auto uuid = UUID(entity);

    entity.SetName("After");

    ASSERT_NO_FATAL_FAILURE(SwitchTo("b.json"));

    const auto id = FindUUID(uuid);
    ASSERT_NE(id, ECS::INVALID_ENTITY_ID);

    ECS::Entity loaded{id, &Registry()};

    EXPECT_EQ(UUID(loaded), uuid);
    EXPECT_EQ(loaded.GetName(), "After");
    EXPECT_TRUE(loaded.HasComponent<ECS::Components::PersistentTagComponent>());
}

TEST_F(SceneTests, PersistentSubtreeRetainsHierarchyWithoutDuplicates) {
    ASSERT_NO_FATAL_FAILURE(SwitchTo("a.json"));

    auto parent = CreateEntity("Parent", true);
    auto child = CreateEntity("Child", false);
    auto grandchild = CreateEntity("Grandchild", true);

    const auto parentUUID = UUID(parent);
    const auto childUUID = UUID(child);
    const auto grandchildUUID = UUID(grandchild);

    Registry().SetParentDirect(static_cast<ECS::EntityID>(child), static_cast<ECS::EntityID>(parent));

    Registry().SetParentDirect(static_cast<ECS::EntityID>(grandchild), static_cast<ECS::EntityID>(child));

    for (const char *destination : {"b.json", "c.json"}) {
        SCOPED_TRACE(destination);

        ASSERT_NO_FATAL_FAILURE(SwitchTo(destination));

        const auto parentID = FindUUID(parentUUID);
        const auto childID = FindUUID(childUUID);
        const auto grandchildID = FindUUID(grandchildUUID);

        ASSERT_NE(parentID, ECS::INVALID_ENTITY_ID);
        ASSERT_NE(childID, ECS::INVALID_ENTITY_ID);
        ASSERT_NE(grandchildID, ECS::INVALID_ENTITY_ID);

        EXPECT_EQ(CountUUID(parentUUID), 1u);
        EXPECT_EQ(CountUUID(childUUID), 1u);
        EXPECT_EQ(CountUUID(grandchildUUID), 1u);

        ECS::Entity loadedChild{childID, &Registry()};
        ECS::Entity loadedGrandchild{grandchildID, &Registry()};

        EXPECT_EQ(static_cast<ECS::EntityID>(loadedChild.GetParent()), parentID);

        EXPECT_EQ(static_cast<ECS::EntityID>(loadedGrandchild.GetParent()), childID);
    }
}

TEST_F(SceneTests, RejectedScenePathsLeaveCurrentSceneUsable) {
    ASSERT_NO_FATAL_FAILURE(SwitchTo("a.json"));

    auto entity = CreateEntity("Keep Me", true);
    const auto uuid = UUID(entity);
    auto *originalScene = manager.GetCurrentScene();

    for (const char *path : {"", "scene.txt"}) {
        SCOPED_TRACE(path);

        ASSERT_NO_THROW(manager.LoadSceneByPath(path));

        EXPECT_EQ(manager.GetCurrentScene(), originalScene);
        EXPECT_EQ(manager.GetCurrentScenePath(), "a.json");
        EXPECT_EQ(CountUUID(uuid), 1u);
    }
}

TEST_F(SceneTests, MissingOrMalformedSceneFilesReturnFailure) {
    {
        Scenes::Scene scene{&context, Scenes::SceneProperties{}};

        bool result = true;

        ASSERT_NO_THROW(result = IO::SceneIO::Deserialize("missing.json", scene));

        EXPECT_FALSE(result);
        EXPECT_TRUE(scene.GetRegistry().GetLivingEntities().empty());
    }

    {
        std::ofstream file(directory / "broken.json");
        ASSERT_TRUE(file.is_open());

        file << "{ invalid json";
        file.close();

        ASSERT_FALSE(file.fail());
    }

    {
        Scenes::Scene scene{&context, Scenes::SceneProperties{}};

        bool result = true;

        ASSERT_NO_THROW(result = IO::SceneIO::Deserialize("broken.json", scene));

        EXPECT_FALSE(result);
        EXPECT_TRUE(scene.GetRegistry().GetLivingEntities().empty());
    }
}
