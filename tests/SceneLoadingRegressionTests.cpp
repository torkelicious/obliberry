#include "TemporaryProject.h"
#include "Core/EngineContext.h"
#include "Core/ResourceManager.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/PersistentTagComponent.h"
#include "ECS/Components/RelationshipComponent.h"
#include "IO/Loaders/EntityFactory.h"
#include "IO/SceneSerialization.h"
#include "Platform/Threading/ThreadPool.h"
#include "Scenes/SceneManager.h"
#include <ObSL/ScriptRuntime.h>
#include <cstdint>
#include <limits>
#include <memory>

class SceneLoadingRegressionTests : public TemporaryProject {
protected:
    Core::EngineContext context;
    Scenes::SceneManager manager;
    ObSL::ScriptRuntime runtime;
    std::unique_ptr<Platform::Threading::ThreadPool> threads;
    Scenes::Scene *original = nullptr;
    UI::UIElement *originalUi = nullptr;
    ECS::EntityID originalEntity = ECS::INVALID_ENTITY_ID;
    Core::UUID originalUuid;

    static nlohmann::json Document(const nlohmann::json &entities = nlohmann::json::array()) {
        return {{"properties", {{"lighting", false}}}, {"entities", entities}, {"PostProcessing", nlohmann::json::array()}, {"ui", {{"elements", {{{"type", "Rect"}, {"name", "Original UI"}}}}}}};
    }

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(TemporaryProject::SetUp());
        context.resources = &Core::ResourceManager::GetInstance();
        context.sceneManager = &manager;
        context.isEditorMode = true;
        manager.SetContext(context);
        ASSERT_NO_FATAL_FAILURE(WriteJson("original.json", Document()));
        ASSERT_TRUE(manager.LoadSceneByPath("original.json"));
        original = manager.GetCurrentScene();
        originalUi = original->GetUISystem().FindByName("Original UI");
        ASSERT_NE(originalUi, nullptr);
        originalEntity = original->GetRegistry().CreateEntity();
        original->GetRegistry().SetEntityName(originalEntity, "Keep Me");
        original->GetRegistry().AddComponent<ECS::Components::TransformComponent>(originalEntity);
        originalUuid = original->GetRegistry().GetEntityUUID(originalEntity);
        original->MarkAsChanged();
    }

    void TearDown() override {
        manager.ClearCurrentScene();
        runtime.shutdown();
        threads.reset();
        context.resources->ClearProjectResources();
        TemporaryProject::TearDown();
    }

    void ExpectOriginalPreserved() {
        ASSERT_EQ(manager.GetCurrentScene(), original);
        EXPECT_EQ(manager.GetCurrentScenePath(), "original.json");
        EXPECT_TRUE(original->HasUnsavedChanges());
        EXPECT_TRUE(original->GetRegistry().IsValid(originalEntity));
        EXPECT_EQ(original->GetRegistry().GetEntityUUID(originalEntity), originalUuid);
        EXPECT_EQ(original->GetUISystem().FindByName("Original UI"), originalUi);
        EXPECT_EQ(context.uiSystem, &original->GetUISystem());
    }
};

TEST_F(SceneLoadingRegressionTests, MissingAndMalformedScenesPreserveActiveScene) {
    ASSERT_NO_FATAL_FAILURE(Write("broken.json", "{invalid json"));
    for (const auto *path : {"missing.json", "broken.json"}) {
        SCOPED_TRACE(path);
        EXPECT_FALSE(manager.LoadSceneByPath(path));
        ASSERT_NO_FATAL_FAILURE(ExpectOriginalPreserved());
    }
}

TEST_F(SceneLoadingRegressionTests, MissingMapAndAssetPreserveActiveScene) {
    auto missingMap = Document();
    missingMap["grid"]["map_file"] = "assets/maps/missing.obmap";
    auto missingAsset = Document();
    missingAsset["ui"]["elements"] = {{{"type", "Image"}, {"texture", "missing-catalog-texture"}}};
    ASSERT_NO_FATAL_FAILURE(WriteJson("missing-map.json", missingMap));
    ASSERT_NO_FATAL_FAILURE(WriteJson("missing-asset.json", missingAsset));
    for (const auto *path : {"missing-map.json", "missing-asset.json"}) {
        SCOPED_TRACE(path);
        EXPECT_FALSE(manager.LoadSceneByPath(path));
        ASSERT_NO_FATAL_FAILURE(ExpectOriginalPreserved());
    }
}

TEST_F(SceneLoadingRegressionTests, InvalidParentValuesPreserveActiveScene) {
    for (const nlohmann::json value :
            {nlohmann::json(-1), nlohmann::json(99), nlohmann::json(0.5), nlohmann::json("0"), nlohmann::json(nullptr), nlohmann::json(true), nlohmann::json(std::numeric_limits<uint64_t>::max())}) {
        SCOPED_TRACE(value.dump());
        ASSERT_NO_FATAL_FAILURE(WriteJson("invalid-parent.json", Document(nlohmann::json::array({{{"parent", value}}}))));
        EXPECT_FALSE(manager.LoadSceneByPath("invalid-parent.json"));
        ASSERT_NO_FATAL_FAILURE(ExpectOriginalPreserved());
    }
}

TEST_F(SceneLoadingRegressionTests, SelfAndAncestorCyclesPreserveActiveScene) {
    const nlohmann::json cycles[] = {nlohmann::json::array({{{"parent", 0}}}), nlohmann::json::array({{{"parent", 1}}, {{"parent", 0}}}), nlohmann::json::array({{{"parent", 1}}, {{"parent", 2}}, {{"parent", 0}}})};
    for (const auto &entities : cycles) {
        SCOPED_TRACE(entities.dump());
        ASSERT_NO_FATAL_FAILURE(WriteJson("cycle.json", Document(entities)));
        EXPECT_FALSE(manager.LoadSceneByPath("cycle.json"));
        ASSERT_NO_FATAL_FAILURE(ExpectOriginalPreserved());
    }
}

TEST_F(SceneLoadingRegressionTests, ParentReferencesToFailedEntitiesRejectLoad) {
    const nlohmann::json entities = nlohmann::json::array({{{"name", 42}, {"components", nlohmann::json::object()}}, {{"parent", 0}}});
    ASSERT_NO_FATAL_FAILURE(WriteJson("failed-parent.json", Document(entities)));
    EXPECT_FALSE(manager.LoadSceneByPath("failed-parent.json"));
    ASSERT_NO_FATAL_FAILURE(ExpectOriginalPreserved());
}

TEST_F(SceneLoadingRegressionTests, ValidForwardParentReferencesLoadAndDeleteAsSubtree) {
    const nlohmann::json entities = nlohmann::json::array({{{"name", "Child"}, {"parent", 2}}, {{"name", "Grandchild"}, {"parent", 0}}, {{"name", "Root"}}});
    ASSERT_NO_FATAL_FAILURE(WriteJson("forward.json", Document(entities)));
    ASSERT_TRUE(manager.LoadSceneByPath("forward.json"));
    auto &registry = manager.GetCurrentScene()->GetRegistry();
    const auto ids = registry.GetLivingEntities();
    ASSERT_EQ(ids.size(), 3u);
    ASSERT_NE(registry.GetComponent<ECS::Components::RelationshipComponent>(ids[0]), nullptr);
    EXPECT_EQ(registry.GetComponent<ECS::Components::RelationshipComponent>(ids[0])->parent, ids[2]);
    EXPECT_EQ(registry.GetComponent<ECS::Components::RelationshipComponent>(ids[1])->parent, ids[0]);
    registry.DestroyEntity(ids[2]);
    EXPECT_TRUE(registry.GetLivingEntities().empty());
}

TEST_F(SceneLoadingRegressionTests, MigrationAssetFailurePreservesSceneAndUi) {
    auto &registry = original->GetRegistry();
    registry.AddComponent<ECS::Components::PersistentTagComponent>(originalEntity);
    auto mesh = context.resources->Register<Rendering::Mesh>("missing-catalog-mesh", std::make_shared<Rendering::Mesh>(Rendering::MeshData{}));
    registry.AddComponent<ECS::Components::MeshComponent>(originalEntity).mesh = mesh;
    auto next = Document();
    next["ui"]["elements"][0]["name"] = "Replacement UI";
    ASSERT_NO_FATAL_FAILURE(WriteJson("migration.json", next));
    EXPECT_FALSE(manager.LoadSceneByPath("migration.json"));
    ASSERT_NO_FATAL_FAILURE(ExpectOriginalPreserved());
    EXPECT_EQ(original->GetUISystem().FindByName("Replacement UI"), nullptr);
}

TEST_F(SceneLoadingRegressionTests, SuccessfulSwitchWorksWithScriptWorkersAndEmptyDestroyQueue) {
    threads = std::make_unique<Platform::Threading::ThreadPool>(2);
    runtime.init("assets/scripts", 2);
    context.scriptPool = &runtime;
    context.threadPool = threads.get();
    ASSERT_NO_FATAL_FAILURE(WriteJson("next.json", Document()));
    ASSERT_TRUE(manager.LoadSceneByPath("next.json"));
    EXPECT_NE(manager.GetCurrentScene(), original);
    EXPECT_EQ(manager.GetCurrentScenePath(), "next.json");
    EXPECT_EQ(context.uiSystem, &manager.GetCurrentScene()->GetUISystem());
    EXPECT_GT(runtime.worker_count(), 0u);
}

TEST_F(SceneLoadingRegressionTests, MapWriteFailurePropagatesAndPreservesSceneDirtyState) {
    auto &map = original->GetRegistry().AddComponent<ECS::Components::MapComponent>(originalEntity);
    map.mapFilePath = "blocked/map.obmap";
    map.grid.EmplaceTile({0, 0}, 1, true);
    ASSERT_NO_FATAL_FAILURE(Write("blocked", "This is a file, not a directory."));
    const auto before = IO::VFS::ReadVirtual("original.json");
    EXPECT_FALSE(manager.SaveCurrentScene());
    EXPECT_TRUE(original->HasUnsavedChanges());
    EXPECT_EQ(IO::VFS::ReadVirtual("original.json"), before);
}
