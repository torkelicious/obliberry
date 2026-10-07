#include "TemporaryProject.h"
#include "Applications/Editor/Commands/EditorCommands.h"
#include "Applications/Editor/Commands/UndoManager.h"
#include "Core/ResourceManager.h"
#include "ECS/Components/MovementComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "TestingUtils.h"
#include <memory>

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
};

TEST_F(EditorCommandTests, TransformExecuteUndoRedoRestoresValues) {
    const auto id = Registry().CreateEntity();
    Registry().AddComponent<ECS::Components::TransformComponent>(id);
    const glm::vec3 before{0.0f};
    const glm::vec3 after{3.0f, 4.0f, 5.0f};
    undo.Execute(std::make_unique<Editor::Commands::TranslateEntityCommand>(id, before, after), context);
    const auto position = [&] { return Registry().GetComponent<ECS::Components::TransformComponent>(id)->transform.GetPosition(); };
    TestUtils::GLM_VecExpectFloat(position(), after);
    undo.Undo(context);
    TestUtils::GLM_VecExpectFloat(position(), before);
    undo.Redo(context);
    TestUtils::GLM_VecExpectFloat(position(), after);

    undo.Execute(std::make_unique<Editor::Commands::RotateEntityCommand>(id, before, after), context);
    TestUtils::GLM_VecExpectFloat(Registry().GetComponent<ECS::Components::TransformComponent>(id)->transform.GetRotation(), after);
    undo.Undo(context);
    TestUtils::GLM_VecExpectFloat(Registry().GetComponent<ECS::Components::TransformComponent>(id)->transform.GetRotation(), before);
    undo.Redo(context);
    TestUtils::GLM_VecExpectFloat(Registry().GetComponent<ECS::Components::TransformComponent>(id)->transform.GetRotation(), after);

    undo.Execute(std::make_unique<Editor::Commands::ScaleEntityCommand>(id, glm::vec3{1}, after), context);
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
    undo.Execute(std::make_unique<Editor::Commands::AddComponentCommand<ECS::Components::MovementComponent>>(id, expected), context);
    ASSERT_TRUE(Registry().HasComponent<ECS::Components::MovementComponent>(id));
    undo.Undo(context);
    EXPECT_FALSE(Registry().HasComponent<ECS::Components::MovementComponent>(id));
    undo.Redo(context);
    ASSERT_TRUE(Registry().HasComponent<ECS::Components::MovementComponent>(id));
    undo.Execute(std::make_unique<Editor::Commands::RemoveComponentCommand<ECS::Components::MovementComponent>>(id, expected), context);
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
    undo.Execute(std::make_unique<Editor::Commands::TranslateEntityCommand>(id, glm::vec3{0}, glm::vec3{3}), context);
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
    undo.Execute(std::make_unique<Editor::Commands::TranslateEntityCommand>(id, glm::vec3{0}, glm::vec3{1}), context);
    undo.Undo(context);
    ASSERT_TRUE(undo.CanRedo());
    undo.Execute(std::make_unique<Editor::Commands::TranslateEntityCommand>(id, glm::vec3{0}, glm::vec3{2}), context);
    EXPECT_FALSE(undo.CanRedo());
    undo.Clear();
    EXPECT_FALSE(undo.CanUndo());
}

TEST_F(EditorCommandTests, DestroyedComponentTargetsAreSafeDuringUndoAndRedo) {
    const auto removed = Registry().CreateEntity();
    const ECS::Components::MovementComponent data;
    Registry().AddComponent<ECS::Components::MovementComponent>(removed, data);
    undo.Execute(std::make_unique<Editor::Commands::RemoveComponentCommand<ECS::Components::MovementComponent>>(removed, data), context);
    Registry().DestroyEntity(removed);
    EXPECT_NO_THROW(undo.Undo(context));
    EXPECT_NO_THROW(undo.Redo(context));
    undo.Clear();

    const auto added = Registry().CreateEntity();
    undo.Execute(std::make_unique<Editor::Commands::AddComponentCommand<ECS::Components::MovementComponent>>(added, data), context);
    Registry().DestroyEntity(added);
    const auto replacement = Registry().CreateEntity();
    EXPECT_NO_THROW(undo.Undo(context));
    EXPECT_NO_THROW(undo.Redo(context));
    EXPECT_FALSE(Registry().HasComponent<ECS::Components::MovementComponent>(replacement));
}
