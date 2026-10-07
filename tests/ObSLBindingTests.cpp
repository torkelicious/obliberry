#include "TemporaryProject.h"
#include "Core/EngineContext.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/ScriptComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Registry.h"
#include "ECS/Systems/Collision/CollisionWorld.h"
#include "ECS/Systems/HierarchySystem.h"
#include "ECS/Systems/ScriptSystem.h"
#include "Scripting/EngineLib/EngineLib.h"
#include "Scripting/EngineLib/EngineLibFactories.h"
#include "Scripting/EngineLib/ScriptCommandBuffer.h"
#include "SaveData/SaveGameManager.h"
#include "TestingUtils.h"
#include <ObSL/ScriptRuntime.h>
#include <deque>
#include <memory>
#include <variant>

class ObSLBindingTests : public TemporaryProject {
protected:
    std::deque<std::string> sources;
    std::vector<std::vector<std::unique_ptr<ObSL::Stmt>>> programs;
    ECS::Registry registry;
    Core::EngineContext context;
    Saves::SaveGameManager saves;
    Platform::Threading::ThreadPool threads{1};
    ObSL::ScriptRuntime runtime;
    Scripting::ScriptCommandBuffer commands;

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(TemporaryProject::SetUp());
        saves.Configure(directory);
        context.saveGameManager = &saves;
        context.scriptPool = &runtime;
        context.threadPool = &threads;
        runtime.init(directory.string(), 1);
        Scripting::EngineLib library;
        library.register_enginelib(Interpreter(), registry, context);
        runtime.get_worker(0)->set_frame_context(&commands);
    }

    void TearDown() override {
        runtime.get_worker(0)->clear_frame_context();
        Scripting::EntityWrapperCache::ClearAll();
        runtime.shutdown();
        saves.Reset();
        TemporaryProject::TearDown();
    }

    ObSL::Interpreter &Interpreter() { return runtime.get_worker(0)->GetInterpreter(); }

    void Run(const std::string &source) {
        sources.push_back(source);
        ObSL::Lexer lexer{sources.back()};
        ObSL::Parser parser{lexer.tokenize()};
        programs.push_back(parser.parse());
        Interpreter().execute_block(programs.back(), Interpreter().get_global_environment());
    }

    ECS::EntityID MakeEntity(const std::string &variable, const glm::vec3 position) {
        const auto id = registry.CreateEntity();
        registry.AddComponent<ECS::Components::TransformComponent>(id).transform.SetPosition(position);
        registry.AddComponent<ECS::Components::ColliderComponent>(id);
        Interpreter().get_global_environment()->define(variable,
            Scripting::CreateEntityObject(&Interpreter(), registry, id));
        return id;
    }
};

TEST_F(ObSLBindingTests, MovementBlocksAndTeleportBypassesCollision) {
    const auto moving = MakeEntity("actor", {-3, 0, 0});
    MakeEntity("wall", {0, 0, 0});
    ECS::Systems::HierarchySystem::Propagate(registry);
    ASSERT_NO_THROW(Run("var transform = actor.GetComponent(\"Transform\"); transform.TryMoveTo(0, 0, 0);"));
    commands.flush(registry);
    TestUtils::GLM_VecExpectFloat(
        registry.GetComponent<ECS::Components::TransformComponent>(moving)->transform.GetPosition(), glm::vec3{-3, 0, 0});

    runtime.get_worker(0)->set_frame_context(&commands);
    ASSERT_NO_THROW(Run("transform.TryMoveTo(-2, 0, 0);"));
    commands.flush(registry);
    TestUtils::GLM_VecExpectFloat(
        registry.GetComponent<ECS::Components::TransformComponent>(moving)->transform.GetPosition(), glm::vec3{-2, 0, 0});

    runtime.get_worker(0)->set_frame_context(&commands);
    ASSERT_NO_THROW(Run("transform.SetPosition(0, 0, 0);"));
    commands.flush(registry);
    TestUtils::GLM_VecExpectFloat(
        registry.GetComponent<ECS::Components::TransformComponent>(moving)->transform.GetPosition(), glm::vec3{0});
}

TEST_F(ObSLBindingTests, SaveBindingsPersistAndRestoreValues) {
    ASSERT_NO_THROW(Run(R"obsl(
        var setOK = save_set("score", 42);
        var filename = save_create("Binding Test");
        save_set("score", 99);
        var loaded = save_load(filename);
        var restored = save_get("score");
        var bad = save_set("", 1);
    )obsl"));
    auto globals = Interpreter().get_global_environment();
    EXPECT_EQ(globals->get("setOK"), ObSL::Value{true});
    EXPECT_EQ(globals->get("loaded"), ObSL::Value{true});
    EXPECT_EQ(globals->get("restored"), ObSL::Value{42.0});
    EXPECT_EQ(globals->get("bad"), ObSL::Value{false});
}

TEST_F(ObSLBindingTests, DestroyedWrappersFailSafelyAndDoNotModifyReplacement) {
    const auto old = MakeEntity("old", {0, 0, 0});
    ASSERT_NO_THROW(Run("var oldTransform = old.GetComponent(\"Transform\");"));
    registry.DestroyEntity(old);
    const auto replacement = MakeEntity("replacement", {7, 8, 9});
    ASSERT_NO_THROW(Run("var oldName = old.GetName(); old.SetName(\"Bad\"); oldTransform.SetPosition(1, 2, 3);"));
    commands.flush(registry);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(Interpreter().get_global_environment()->get("oldName")));
    TestUtils::GLM_VecExpectFloat(
        registry.GetComponent<ECS::Components::TransformComponent>(replacement)->transform.GetPosition(), glm::vec3{7, 8, 9});
    EXPECT_NE(registry.GetEntityName(replacement), "Bad");
}

TEST_F(ObSLBindingTests, CollisionEnterStayExitCallbacksReachBothEntitiesOnce) {
    const auto first = MakeEntity("first", {0, 0, 0});
    const auto second = MakeEntity("second", {0.25f, 0, 0});
    ASSERT_NO_THROW(Run(R"obsl(
        var entered = 0;
        var stayed = 0;
        var exited = 0;
        fn enter(other) { entered++; }
        fn stay(other) { stayed++; }
        fn exit(other) { exited++; }
    )obsl"));
    auto env = Interpreter().get_global_environment();
    for (const auto id : {first, second}) {
        auto &script = registry.AddComponent<ECS::Components::ScriptComponent>(id);
        script.slots.resize(1);
        auto &slot = script.slots[0];
        slot.isInitialized = true;
        slot.instance_envs = {env};
        slot.on_collision_enter_functions = {std::get<ObSL::ObSLCallable *>(env->get("enter"))};
        slot.on_collision_stay_functions = {std::get<ObSL::ObSLCallable *>(env->get("stay"))};
        slot.on_collision_exit_functions = {std::get<ObSL::ObSLCallable *>(env->get("exit"))};
    }
    ECS::Collision::CollisionWorld world;
    const auto dispatch = [&] {
        ECS::Systems::HierarchySystem::Propagate(registry);
        world.Update(registry, {});
        ECS::Systems::ScriptSystem::DispatchCollisionEvents(registry, context, world.GetEvents(), {});
    };
    dispatch();
    EXPECT_EQ(env->get("entered"), ObSL::Value{2.0});
    dispatch();
    EXPECT_EQ(env->get("stayed"), ObSL::Value{2.0});
    registry.GetComponent<ECS::Components::TransformComponent>(second)->transform.SetPosition({5, 0, 0});
    dispatch();
    EXPECT_EQ(env->get("exited"), ObSL::Value{2.0});
    dispatch();
    EXPECT_EQ(env->get("entered"), ObSL::Value{2.0});
    EXPECT_EQ(env->get("stayed"), ObSL::Value{2.0});
    EXPECT_EQ(env->get("exited"), ObSL::Value{2.0});
}
