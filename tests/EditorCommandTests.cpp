#include "TemporaryProject.h"
#include "Applications/Editor/Commands/EditorCommands.h"
#include "Applications/Editor/Commands/UndoManager.h"
#include "Core/ResourceManager.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "IO/Loaders/EntityFactory.h"
#include "TestingUtils.h"
#include <memory>
#include "Core/Utils/ECSUtils.h"
#include "ECS/Components/MapComponent.h"
#include "ECS/Components/MapStateComponent.h"
#include "ECS/Components/PrefabSourceComponent.h"
#include "ECS/Components/PersistentTagComponent.h"
#include "ECS/Components/CustomDataComponent.h"
#include "ECS/Components/DestroyTagComponent.h"
#include <stdexcept>

class EditorCommandTests : public TemporaryProject {
protected:
    Core::EngineContext context;
    Scenes::SceneManager scenes;
    Editor::Commands::UndoManager undo;

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(TemporaryProject::SetUp());
        context.resources = &Core::ResourceManager::GetInstance();
        context.sceneManager = &scenes;
        context.isEditorMode = true;
        scenes.SetContext(context);
        ASSERT_NO_FATAL_FAILURE(WriteJson("scene.json", {{"properties", {{"lighting", false}}}, {"entities", nlohmann::json::array()}, {"PostProcessing", nlohmann::json::array()}}));
        scenes.LoadSceneByPath("scene.json");
        ASSERT_NE(scenes.GetCurrentScene(), nullptr);
    }

    void TearDown() override {
        undo.Clear();
        scenes.ClearCurrentScene();
        TemporaryProject::TearDown();
    }

    ECS::Registry &Registry() { return scenes.GetCurrentScene()->GetRegistry(); }
    ECS::EntityID Find(const std::string &uuid) {
        for (const auto id : Registry().GetLivingEntities()) {
            if (Registry().GetEntityUUID(id) == uuid) {
                return id;
            }
        }
        return ECS::INVALID_ENTITY_ID;
    }
};

TEST_F(EditorCommandTests, TransformExecuteUndoRedoRestoresValues) {
    const auto id = Registry().CreateEntity();
    Registry().AddComponent<ECS::Components::TransformComponent>(id);
    const glm::vec3 before{0.0f};
    const glm::vec3 after{3.0f, 4.0f, 5.0f};
    undo.Execute(std::make_unique<Editor::Commands::TranslateEntityCommand>(Registry().GetEntityUUID(id), before, after), context);
    const auto position = [&] { return Registry().GetComponent<ECS::Components::TransformComponent>(id)->transform.GetPosition(); };
    TestUtils::GLM_VecExpectFloat(position(), after);
    undo.Undo(context);
    TestUtils::GLM_VecExpectFloat(position(), before);
    undo.Redo(context);
    TestUtils::GLM_VecExpectFloat(position(), after);

    undo.Execute(std::make_unique<Editor::Commands::RotateEntityCommand>(Registry().GetEntityUUID(id), before, after), context);
    TestUtils::GLM_VecExpectFloat(Registry().GetComponent<ECS::Components::TransformComponent>(id)->transform.GetRotation(), after);
    undo.Undo(context);
    TestUtils::GLM_VecExpectFloat(Registry().GetComponent<ECS::Components::TransformComponent>(id)->transform.GetRotation(), before);
    undo.Redo(context);
    TestUtils::GLM_VecExpectFloat(Registry().GetComponent<ECS::Components::TransformComponent>(id)->transform.GetRotation(), after);

    undo.Execute(std::make_unique<Editor::Commands::ScaleEntityCommand>(Registry().GetEntityUUID(id), glm::vec3{1}, after), context);
    TestUtils::GLM_VecExpectFloat(Registry().GetComponent<ECS::Components::TransformComponent>(id)->transform.GetScale(), after);
    undo.Undo(context);
    TestUtils::GLM_VecExpectFloat(Registry().GetComponent<ECS::Components::TransformComponent>(id)->transform.GetScale(), glm::vec3{1});
    undo.Redo(context);
    TestUtils::GLM_VecExpectFloat(Registry().GetComponent<ECS::Components::TransformComponent>(id)->transform.GetScale(), after);
}

TEST_F(EditorCommandTests, AddRemoveComponentUndoRedoRestoresData) {
    const auto id = Registry().CreateEntity();
    ECS::Components::MovementComponent expected;
    expected.timePerStep = 0.75f;
    expected.autoMove = true;
    undo.Execute(std::make_unique<Editor::Commands::AddComponentCommand<ECS::Components::MovementComponent>>(Registry().GetEntityUUID(id), expected), context);
    ASSERT_TRUE(Registry().HasComponent<ECS::Components::MovementComponent>(id));
    undo.Undo(context);
    EXPECT_FALSE(Registry().HasComponent<ECS::Components::MovementComponent>(id));
    undo.Redo(context);
    ASSERT_TRUE(Registry().HasComponent<ECS::Components::MovementComponent>(id));
    undo.Execute(std::make_unique<Editor::Commands::RemoveComponentCommand<ECS::Components::MovementComponent>>(Registry().GetEntityUUID(id), expected), context);
    EXPECT_FALSE(Registry().HasComponent<ECS::Components::MovementComponent>(id));
    undo.Undo(context);
    const auto *actual = Registry().GetComponent<ECS::Components::MovementComponent>(id);
    ASSERT_NE(actual, nullptr);
    EXPECT_FLOAT_EQ(actual->timePerStep, expected.timePerStep);
    EXPECT_TRUE(actual->autoMove);
}

TEST_F(EditorCommandTests, DestroyedTransformTargetDoesNotModifyReusedEntity) {
    const auto id = Registry().CreateEntity();
    Registry().AddComponent<ECS::Components::TransformComponent>(id);
    undo.Execute(std::make_unique<Editor::Commands::TranslateEntityCommand>(Registry().GetEntityUUID(id), glm::vec3{0}, glm::vec3{3}), context);
    Registry().DestroyEntity(id);
    const auto replacement = Registry().CreateEntity();
    auto &transform = Registry().AddComponent<ECS::Components::TransformComponent>(replacement);
    transform.transform.SetPosition({7, 8, 9});
    EXPECT_NO_THROW(undo.Undo(context));
    EXPECT_NO_THROW(undo.Redo(context));
    TestUtils::GLM_VecExpectFloat(transform.transform.GetPosition(), glm::vec3{7, 8, 9});
}

TEST_F(EditorCommandTests, NewCommandClearsRedoAndEmptyHistoryIsSafe) {
    EXPECT_NO_THROW(undo.Undo(context));
    EXPECT_NO_THROW(undo.Redo(context));
    const auto id = Registry().CreateEntity();
    Registry().AddComponent<ECS::Components::TransformComponent>(id);
    undo.Execute(std::make_unique<Editor::Commands::TranslateEntityCommand>(Registry().GetEntityUUID(id), glm::vec3{0}, glm::vec3{1}), context);
    undo.Undo(context);
    ASSERT_TRUE(undo.CanRedo());
    undo.Execute(std::make_unique<Editor::Commands::TranslateEntityCommand>(Registry().GetEntityUUID(id), glm::vec3{0}, glm::vec3{2}), context);
    EXPECT_FALSE(undo.CanRedo());
    undo.Clear();
    EXPECT_FALSE(undo.CanUndo());
}

TEST_F(EditorCommandTests, DestroyedComponentTargetsAreSafeDuringUndoAndRedo) {
    const auto removed = Registry().CreateEntity();
    const ECS::Components::MovementComponent data;
    Registry().AddComponent<ECS::Components::MovementComponent>(removed, data);
    undo.Execute(std::make_unique<Editor::Commands::RemoveComponentCommand<ECS::Components::MovementComponent>>(Registry().GetEntityUUID(removed), data), context);
    Registry().DestroyEntity(removed);
    EXPECT_NO_THROW(undo.Undo(context));
    EXPECT_NO_THROW(undo.Redo(context));
    undo.Clear();

    const auto added = Registry().CreateEntity();
    undo.Execute(std::make_unique<Editor::Commands::AddComponentCommand<ECS::Components::MovementComponent>>(Registry().GetEntityUUID(added), data), context);
    Registry().DestroyEntity(added);
    const auto replacement = Registry().CreateEntity();
    EXPECT_NO_THROW(undo.Undo(context));
    EXPECT_NO_THROW(undo.Redo(context));
    EXPECT_FALSE(Registry().HasComponent<ECS::Components::MovementComponent>(replacement));
}

TEST_F(EditorCommandTests, DeleteUndoRestoresSubtreeUUIDsAndSiblingOrder) {
    const auto parent = Registry().CreateEntity();
    const auto before = Registry().CreateEntity();
    const auto root = Registry().CreateEntity();
    const auto after = Registry().CreateEntity();
    const auto child = Registry().CreateEntity();
    const auto leaf = Registry().CreateEntity();
    Registry().SetParentDirect(before, parent);
    Registry().SetParentDirect(root, parent);
    Registry().SetParentDirect(after, parent);
    Registry().SetParentDirect(child, root);
    Registry().SetParentDirect(leaf, child);
    const auto rootUUID = Registry().GetEntityUUID(root);
    const auto childUUID = Registry().GetEntityUUID(child);
    const auto leafUUID = Registry().GetEntityUUID(leaf);
    Registry().SetEntityName(root, "Enemy");
    Registry().AddComponent<ECS::Components::TransformComponent>(root).transform.SetPosition({4, 5, 6});
    Registry().AddComponent<ECS::Components::PersistentTagComponent>(root);
    auto &prefab = Registry().AddComponent<ECS::Components::PrefabSourceComponent>(root);
    prefab.prefabPath = "assets/prefabs/enemy.json";
    prefab.originalData = {{"example", 42}};
    Registry().AddComponent<ECS::Components::MapStateComponent>(child).hasSelection = true;
    Registry().AddComponent<ECS::Components::CustomDataComponent>(leaf);
    Registry().AddComponent<ECS::Components::RelationshipComponent>(before);

    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>(rootUUID), context);
    EXPECT_FALSE(Registry().IsValid(root));
    EXPECT_FALSE(Registry().IsValid(child));
    EXPECT_FALSE(Registry().IsValid(leaf));
    undo.Undo(context);
    const auto restored = Find(rootUUID);
    const auto restoredChild = Find(childUUID);
    const auto restoredLeaf = Find(leafUUID);
    ASSERT_NE(restored, ECS::INVALID_ENTITY_ID);
    ASSERT_NE(restoredChild, ECS::INVALID_ENTITY_ID);
    ASSERT_NE(restoredLeaf, ECS::INVALID_ENTITY_ID);
    EXPECT_NE(restored, root);
    EXPECT_EQ(Registry().GetEntityName(restored), "Enemy");
    TestUtils::GLM_VecExpectFloat(Registry().GetComponent<ECS::Components::TransformComponent>(restored)->transform.GetPosition(), glm::vec3(4, 5, 6));
    EXPECT_TRUE(Registry().HasComponent<ECS::Components::PersistentTagComponent>(restored));
    const auto *restoredPrefab = Registry().GetComponent<ECS::Components::PrefabSourceComponent>(restored);
    ASSERT_NE(restoredPrefab, nullptr);
    EXPECT_EQ(restoredPrefab->prefabPath, "assets/prefabs/enemy.json");
    EXPECT_EQ(restoredPrefab->originalData, nlohmann::json({{"example", 42}}));
    EXPECT_TRUE(Registry().GetComponent<ECS::Components::MapStateComponent>(restoredChild)->hasSelection);
    EXPECT_TRUE(Registry().HasComponent<ECS::Components::CustomDataComponent>(restoredLeaf));
    EXPECT_EQ(Registry().GetComponent<ECS::Components::RelationshipComponent>(parent)->children, (std::vector<ECS::EntityID>{before, restored, after}));
    EXPECT_EQ(Registry().GetComponent<ECS::Components::RelationshipComponent>(restoredChild)->parent, restored);
    EXPECT_EQ(Registry().GetComponent<ECS::Components::RelationshipComponent>(restoredLeaf)->parent, restoredChild);
    undo.Redo(context);
    EXPECT_EQ(Find(rootUUID), ECS::INVALID_ENTITY_ID);
    undo.Undo(context);
    EXPECT_NE(Find(rootUUID), ECS::INVALID_ENTITY_ID);
}

TEST_F(EditorCommandTests, EarlierEditsResolveRestoredEntity) {
    const auto id = Registry().CreateEntity();
    const auto uuid = Registry().GetEntityUUID(id);
    Registry().AddComponent<ECS::Components::TransformComponent>(id);
    auto &movement = Registry().AddComponent<ECS::Components::MovementComponent>(id);
    const float oldSpeed = movement.timePerStep;
    const float newSpeed = 0.9f;
    undo.Execute(std::make_unique<Editor::Commands::TranslateEntityCommand>(uuid, glm::vec3(0), glm::vec3(3)), context);
    undo.Execute(std::make_unique<Editor::Commands::RotateEntityCommand>(uuid, glm::vec3(0), glm::vec3(2)), context);
    undo.Execute(std::make_unique<Editor::Commands::ScaleEntityCommand>(uuid, glm::vec3(1), glm::vec3(4)), context);
    undo.Execute(std::make_unique<Editor::Commands::SetNameCommand>(uuid, "", "Changed"), context);
    undo.Execute(std::make_unique<Editor::Commands::ModifyComponentFieldCommand<ECS::Components::MovementComponent>>(
                         uuid, offsetof(ECS::Components::MovementComponent, timePerStep), sizeof(float), &oldSpeed, &newSpeed, "Speed"),
            context);
    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>(uuid), context);
    undo.Undo(context);
    const auto restored = Find(uuid);
    ASSERT_NE(restored, ECS::INVALID_ENTITY_ID);
    undo.Undo(context);
    EXPECT_FLOAT_EQ(Registry().GetComponent<ECS::Components::MovementComponent>(restored)->timePerStep, oldSpeed);
    undo.Undo(context);
    EXPECT_EQ(Registry().GetEntityName(restored), "");
    undo.Undo(context);
    TestUtils::GLM_VecExpectFloat(Registry().GetComponent<ECS::Components::TransformComponent>(restored)->transform.GetScale(), glm::vec3(1));
    undo.Undo(context);
    TestUtils::GLM_VecExpectFloat(Registry().GetComponent<ECS::Components::TransformComponent>(restored)->transform.GetRotation(), glm::vec3(0));
    undo.Undo(context);
    TestUtils::GLM_VecExpectFloat(Registry().GetComponent<ECS::Components::TransformComponent>(restored)->transform.GetPosition(), glm::vec3(0));
    for (int i = 0; i < 5; ++i) {
        undo.Redo(context);
    }
    EXPECT_FLOAT_EQ(Registry().GetComponent<ECS::Components::MovementComponent>(restored)->timePerStep, newSpeed);
    EXPECT_EQ(Registry().GetEntityName(restored), "Changed");
}

TEST_F(EditorCommandTests, ComponentCommandsResolveRestoredEntity) {
    const auto id = Registry().CreateEntity();
    const auto uuid = Registry().GetEntityUUID(id);
    ECS::Components::MovementComponent data;
    data.timePerStep = 0.8f;
    undo.Execute(std::make_unique<Editor::Commands::AddComponentCommand<ECS::Components::MovementComponent>>(uuid, data), context);
    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>(uuid), context);
    undo.Undo(context);
    undo.Undo(context);
    const auto restored = Find(uuid);
    EXPECT_FALSE(Registry().HasComponent<ECS::Components::MovementComponent>(restored));
    undo.Redo(context);
    ASSERT_TRUE(Registry().HasComponent<ECS::Components::MovementComponent>(restored));
    undo.Execute(std::make_unique<Editor::Commands::RemoveComponentCommand<ECS::Components::MovementComponent>>(uuid, data), context);
    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>(uuid), context);
    undo.Undo(context);
    undo.Undo(context);
    EXPECT_FLOAT_EQ(Registry().GetComponent<ECS::Components::MovementComponent>(Find(uuid))->timePerStep, 0.8f);
}

TEST_F(EditorCommandTests, ScriptCommandsResolveRestoredAndMovedComponents) {
    const auto other = Registry().CreateEntity();
    Registry().AddComponent<ECS::Components::ScriptComponent>(other);
    const auto id = Registry().CreateEntity();
    const auto uuid = Registry().GetEntityUUID(id);
    undo.Execute(std::make_unique<Editor::Commands::AddScriptCommand>(uuid, "first.obsl"), context);
    Registry().DestroyEntity(other); // Moves the target's component in the dense pool.
    undo.Execute(std::make_unique<Editor::Commands::AddScriptCommand>(uuid, "second.obsl"), context);
    undo.Execute(std::make_unique<Editor::Commands::RemoveScriptCommand>(uuid, 0), context);
    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>(uuid), context);
    undo.Undo(context);
    undo.Undo(context);
    auto *scripts = Registry().GetComponent<ECS::Components::ScriptComponent>(Find(uuid));
    ASSERT_NE(scripts, nullptr);
    ASSERT_EQ(scripts->slots.size(), 2u);
    EXPECT_EQ(scripts->slots[0].scriptPath, "first.obsl");
    EXPECT_EQ(scripts->slots[1].scriptPath, "second.obsl");
    undo.Undo(context);
    scripts = Registry().GetComponent<ECS::Components::ScriptComponent>(Find(uuid));
    ASSERT_EQ(scripts->slots.size(), 1u);
    undo.Undo(context);
    EXPECT_FALSE(Registry().HasComponent<ECS::Components::ScriptComponent>(Find(uuid)));
    undo.Redo(context);
    undo.Redo(context);
    undo.Redo(context);
    scripts = Registry().GetComponent<ECS::Components::ScriptComponent>(Find(uuid));
    ASSERT_EQ(scripts->slots.size(), 1u);
    EXPECT_EQ(scripts->slots[0].scriptPath, "second.obsl");
}

TEST_F(EditorCommandTests, RemovingLastScriptRestoresComponentAndMetadata) {
    const auto id = Registry().CreateEntity();
    const auto uuid = Registry().GetEntityUUID(id);
    ECS::Components::ScriptSlot slot;
    slot.scriptPath = "example.obsl";
    slot.source_code = "print 42;";
    Registry().AddComponent<ECS::Components::ScriptComponent>(id).slots.push_back(std::move(slot));
    undo.Execute(std::make_unique<Editor::Commands::RemoveScriptCommand>(uuid, 0), context);
    EXPECT_FALSE(Registry().HasComponent<ECS::Components::ScriptComponent>(id));
    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>(uuid), context);
    undo.Undo(context);
    undo.Undo(context);
    const auto *scripts = Registry().GetComponent<ECS::Components::ScriptComponent>(Find(uuid));
    ASSERT_NE(scripts, nullptr);
    ASSERT_EQ(scripts->slots.size(), 1u);
    EXPECT_EQ(scripts->slots[0].scriptPath, "example.obsl");
    EXPECT_EQ(scripts->slots[0].source_code, "print 42;");
    EXPECT_FALSE(scripts->slots[0].isInitialized);
}

TEST_F(EditorCommandTests, MapPaintingResolvesMovedAndRestoredMap) {
    const auto other = Registry().CreateEntity();
    Registry().AddComponent<ECS::Components::MapComponent>(other);
    const auto id = Registry().CreateEntity();
    const auto uuid = Registry().GetEntityUUID(id);
    auto &map = Registry().AddComponent<ECS::Components::MapComponent>(id);
    map.mapFilePath = "map.obmap";
    map.grid.EmplaceTile({2, 3}, 1, true);
    using Command = Editor::Commands::MapChangeTileCommand;
    Command::StateMap before{{{2, 3}, Command::TileState{1, true}}};
    Command::StateMap after{{{2, 3}, Command::TileState{7, false}}};
    undo.Execute(std::make_unique<Command>(before, after, uuid), context);
    Registry().DestroyEntity(other);
    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>(uuid), context);
    undo.Undo(context);
    undo.Undo(context);
    auto *restored = Registry().GetComponent<ECS::Components::MapComponent>(Find(uuid));
    ASSERT_NE(restored, nullptr);
    ASSERT_NE(restored->grid.Get({2, 3}), nullptr);
    EXPECT_EQ(restored->mapFilePath, "map.obmap");
    EXPECT_EQ(restored->grid.Get({2, 3})->type, 1);
    EXPECT_TRUE(restored->grid.Get({2, 3})->walkable);
    EXPECT_TRUE(restored->needsMeshUpdate);
    EXPECT_TRUE(restored->mapDirty);
    undo.Redo(context);
    EXPECT_EQ(restored->grid.Get({2, 3})->type, 7);
    EXPECT_FALSE(restored->grid.Get({2, 3})->walkable);
}

TEST_F(EditorCommandTests, PasteRedoPreservesRootAndDescendantUUIDsForLaterEdits) {
    const auto original = Registry().CreateEntity();
    const auto child = Registry().CreateEntity();
    Registry().SetEntityName(original, "Original");
    Registry().SetParentDirect(child, original);
    Registry().AddComponent<ECS::Components::TransformComponent>(child);
    const auto data = ECS::Utils::CopyEntityTreeToJson(original, &Registry());
    auto command = std::make_unique<Editor::Commands::PasteEntityCommand>(data);
    const auto pastedUUID = command->GetCreatedUUID();
    undo.Execute(std::move(command), context);
    const auto pasted = Find(pastedUUID);
    ASSERT_NE(pasted, ECS::INVALID_ENTITY_ID);
    ASSERT_NE(pasted, original);
    const auto pastedChild = Registry().GetComponent<ECS::Components::RelationshipComponent>(pasted)->children.at(0);
    const auto childUUID = Registry().GetEntityUUID(pastedChild);
    EXPECT_NE(childUUID, Registry().GetEntityUUID(child));
    undo.Execute(std::make_unique<Editor::Commands::SetNameCommand>(childUUID, "", "Child edit"), context);
    undo.Undo(context);
    undo.Undo(context);
    EXPECT_EQ(Find(pastedUUID), ECS::INVALID_ENTITY_ID);
    undo.Redo(context);
    EXPECT_NE(Find(pastedUUID), ECS::INVALID_ENTITY_ID);
    EXPECT_NE(Find(childUUID), ECS::INVALID_ENTITY_ID);
    undo.Redo(context);
    EXPECT_EQ(Registry().GetEntityName(Find(childUUID)), "Child edit");
    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>(pastedUUID), context);
    undo.Undo(context);
    undo.Undo(context);
    EXPECT_EQ(Registry().GetEntityName(Find(childUUID)), "");
}

TEST_F(EditorCommandTests, SingleEntityPasteAddsCopySuffixOnceAndPreservesEmptyEntities) {
    const auto id = Registry().CreateEntity();
    Registry().SetEntityName(id, "Empty");
    const auto data = ECS::Utils::CopyEntityToJson(id, &Registry());
    auto command = std::make_unique<Editor::Commands::PasteEntityCommand>(data);
    const auto uuid = command->GetCreatedUUID();
    undo.Execute(std::move(command), context);
    ASSERT_NE(Find(uuid), ECS::INVALID_ENTITY_ID);
    EXPECT_EQ(Registry().GetEntityName(Find(uuid)), "Empty (copy)");
    undo.Undo(context);
    undo.Redo(context);
    EXPECT_EQ(Registry().GetEntityName(Find(uuid)), "Empty (copy)");
}

TEST_F(EditorCommandTests, MissingParentFailureKeepsDeletionAvailableForRetry) {
    const auto parent = Registry().CreateEntity();
    const auto parentUUID = Registry().GetEntityUUID(parent);
    const auto child = Registry().CreateEntity();
    const auto childUUID = Registry().GetEntityUUID(child);
    Registry().SetParentDirect(child, parent);
    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>(childUUID), context);
    Registry().DestroyEntity(parent);
    undo.Undo(context);
    EXPECT_TRUE(undo.CanUndo());
    EXPECT_FALSE(undo.CanRedo());
    EXPECT_EQ(Find(childUUID), ECS::INVALID_ENTITY_ID);
    const auto restoredParent = Registry().CreateEntity();
    ASSERT_TRUE(Registry().SetEntityUUID(restoredParent, parentUUID));
    undo.Undo(context);
    EXPECT_FALSE(undo.CanUndo());
    EXPECT_TRUE(undo.CanRedo());
    ASSERT_NE(Find(childUUID), ECS::INVALID_ENTITY_ID);
    EXPECT_EQ(Registry().GetComponent<ECS::Components::RelationshipComponent>(Find(childUUID))->parent, restoredParent);
}

TEST_F(EditorCommandTests, ConflictingDescendantUUIDDoesNotPartiallyRestoreSubtree) {
    const auto root = Registry().CreateEntity();
    const auto child = Registry().CreateEntity();
    const auto uuid = Registry().GetEntityUUID(root);
    const auto childUUID = Registry().GetEntityUUID(child);
    Registry().SetParentDirect(child, root);
    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>(uuid), context);
    const auto conflicting = Registry().CreateEntity();
    ASSERT_TRUE(Registry().SetEntityUUID(conflicting, childUUID));
    const auto count = Registry().GetLivingEntities().size();
    undo.Undo(context);
    EXPECT_EQ(Registry().GetLivingEntities().size(), count);
    EXPECT_EQ(Find(uuid), ECS::INVALID_ENTITY_ID);
    EXPECT_TRUE(undo.CanUndo());
    EXPECT_FALSE(undo.CanRedo());
    Registry().DestroyEntity(conflicting);
    undo.Undo(context);
    EXPECT_NE(Find(uuid), ECS::INVALID_ENTITY_ID);
    EXPECT_NE(Find(childUUID), ECS::INVALID_ENTITY_ID);
}

TEST_F(EditorCommandTests, FailedNewCommandPreservesRedo) {
    const auto id = Registry().CreateEntity();
    Registry().AddComponent<ECS::Components::TransformComponent>(id);
    undo.Execute(std::make_unique<Editor::Commands::TranslateEntityCommand>(Registry().GetEntityUUID(id), glm::vec3(0), glm::vec3(2)), context);
    undo.Undo(context);
    ASSERT_TRUE(undo.CanRedo());
    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>("missing-uuid"), context);
    EXPECT_TRUE(undo.CanRedo());
    EXPECT_FALSE(undo.CanUndo());
    undo.Redo(context);
    TestUtils::GLM_VecExpectFloat(Registry().GetComponent<ECS::Components::TransformComponent>(id)->transform.GetPosition(), glm::vec3(2));
}

TEST_F(EditorCommandTests, UnsupportedCustomFunctionsLeaveEntityAndHistoryUntouched) {
    ObSL::Interpreter interpreter(".");
    const auto id = Registry().CreateEntity();
    const auto uuid = Registry().GetEntityUUID(id);
    const auto function = interpreter.get_global_environment()->get("Object");
    ASSERT_TRUE(std::holds_alternative<ObSL::ObSLCallable *>(function));
    auto &custom = Registry().AddComponent<ECS::Components::CustomDataComponent>(id);
    custom.Set("runtime", function, interpreter);
    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>(uuid), context);
    EXPECT_TRUE(Registry().IsValid(id));
    EXPECT_FALSE(undo.CanUndo());
    ASSERT_TRUE(Registry().HasComponent<ECS::Components::CustomDataComponent>(id));
    EXPECT_EQ(std::get<ObSL::ObSLCallable *>(Registry().GetComponent<ECS::Components::CustomDataComponent>(id)->Get("runtime")), std::get<ObSL::ObSLCallable *>(function));
    Registry().RemoveComponent<ECS::Components::CustomDataComponent>(id);
    Registry().AddComponent<ECS::Components::DestroyTagComponent>(id);
    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>(uuid), context);
    EXPECT_TRUE(Registry().IsValid(id));
    EXPECT_TRUE(Registry().HasComponent<ECS::Components::DestroyTagComponent>(id));
    EXPECT_FALSE(undo.CanUndo());
}

TEST_F(EditorCommandTests, RepeatedDeleteUndoDoesNotAccumulateEntitiesOrAssetReferences) {
    const auto root = Registry().CreateEntity();
    const auto child = Registry().CreateEntity();
    Registry().SetParentDirect(child, root);
    const auto uuid = Registry().GetEntityUUID(root);
    const auto count = Registry().GetLivingEntities().size();
    const auto assets = scenes.GetCurrentScene()->GetRetainedAssetCount();
    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>(uuid), context);
    for (int i = 0; i < 250; ++i) {
        undo.Undo(context);
        ASSERT_EQ(Registry().GetLivingEntities().size(), count);
        ASSERT_NE(Find(uuid), ECS::INVALID_ENTITY_ID);
        undo.Redo(context);
        ASSERT_EQ(Registry().GetLivingEntities().size(), count - 2);
    }
    EXPECT_EQ(scenes.GetCurrentScene()->GetRetainedAssetCount(), assets);
}

TEST_F(EditorCommandTests, EmptyRelationshipComponentSurvivesDeletionUndo) {
    const auto id = Registry().CreateEntity();
    const auto uuid = Registry().GetEntityUUID(id);
    Registry().AddComponent<ECS::Components::RelationshipComponent>(id);
    undo.Execute(std::make_unique<Editor::Commands::DeleteEntityCommand>(uuid), context);
    undo.Undo(context);
    ASSERT_NE(Find(uuid), ECS::INVALID_ENTITY_ID);
    const auto *relationship = Registry().GetComponent<ECS::Components::RelationshipComponent>(Find(uuid));
    ASSERT_NE(relationship, nullptr);
    EXPECT_EQ(relationship->parent, ECS::INVALID_ENTITY_ID);
    EXPECT_TRUE(relationship->children.empty());
}

TEST_F(EditorCommandTests, InvalidScriptIndexAndAbsentSceneAreSafeFailures) {
    const auto id = Registry().CreateEntity();
    const auto uuid = Registry().GetEntityUUID(id);
    Registry().AddComponent<ECS::Components::ScriptComponent>(id);
    undo.Execute(std::make_unique<Editor::Commands::RemoveScriptCommand>(uuid, -1), context);
    EXPECT_FALSE(undo.CanUndo());
    EXPECT_TRUE(Registry().HasComponent<ECS::Components::ScriptComponent>(id));
    context.sceneManager = nullptr;
    undo.Execute(std::make_unique<Editor::Commands::SetNameCommand>(uuid, "", "Changed"), context);
    EXPECT_FALSE(undo.CanUndo());
    EXPECT_EQ(Registry().GetEntityName(id), "");
}

TEST_F(EditorCommandTests, ZeroHistoryLimitExecutesWithoutRetainingCommands) {
    Editor::Commands::UndoManager noHistory(0);
    const auto id = Registry().CreateEntity();
    const auto uuid = Registry().GetEntityUUID(id);
    noHistory.Execute(std::make_unique<Editor::Commands::SetNameCommand>(uuid, "", "Changed"), context);
    EXPECT_EQ(Registry().GetEntityName(id), "Changed");
    EXPECT_FALSE(noHistory.CanUndo());
}
