#include "TemporaryProject.h"
#include "Core/EngineContext.h"
#include "Core/ResourceManager.h"
#include "ECS/Components/ColliderComponent.h"
#include "ECS/Components/DestroyTagComponent.h"
#include "ECS/Components/ScriptComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/HierarchySystem.h"
#include "ECS/Systems/ScriptSystem.h"
#include "IO/Package/Container.h"
#include "Scenes/Scene.h"
#include "Scripting/EngineLib/EngineLibFactories.h"
#include <ObSL/ASTSerializer.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace {
    constexpr ObSL::Token CallToken{.type = ObSL::TokenType::LEFT_PAREN, .lexeme = "(", .line = 0, .column = 0, .start_pos = 0, .end_pos = 0};
    using Hook = std::function<ObSL::Value(ObSL::Interpreter *, const std::vector<ObSL::Value> &)>;

    class TrackedHook : public ObSL::NativeFunction {
        std::atomic<int> *m_Destroyed;

    public:
        explicit TrackedHook(std::atomic<int> *destroyed)
            : NativeFunction(1, [](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { return std::monostate{}; }, "tracked_destroy"), m_Destroyed(destroyed) {}
        ~TrackedHook() override { ++*m_Destroyed; }
    };
}

class ScriptLifecycleTests : public TemporaryProject, public testing::WithParamInterface<size_t> {
protected:
    Core::EngineContext context;
    ObSL::ScriptRuntime runtime;
    std::unique_ptr<Platform::Threading::ThreadPool> threads;
    std::unique_ptr<Scenes::Scene> scene;
    std::atomic<int> releasedHooks{0};

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(TemporaryProject::SetUp());
        ECS::Systems::ScriptSystem::s_PackagedStringPools.clear();
        Scripting::EntityWrapperCache::ClearAll();
        threads = std::make_unique<Platform::Threading::ThreadPool>(GetParam());
        runtime.init("assets/scripts", GetParam());
        ECS::Systems::ScriptSystem::SetupScriptRuntime(runtime);
        context.resources = &Core::ResourceManager::GetInstance();
        context.scriptPool = &runtime;
        context.threadPool = threads.get();
        context.frameCount = 1;
        context.isEditorMode = true;
        scene = std::make_unique<Scenes::Scene>(&context, Scenes::SceneProperties{.ScenePath = "lifecycle.json", .EnableLightingSystem = false});
        scene->OnEnter();
    }

    void TearDown() override {
        scene.reset();
        context.uiSystem = nullptr;
        context.uiCmdBuf = nullptr;
        ECS::Systems::ScriptSystem::s_PackagedStringPools.clear();
        Scripting::EntityWrapperCache::ClearAll();
        runtime.shutdown();
        threads.reset();
        TemporaryProject::TearDown();
    }

    ECS::Registry &Registry() { return scene->GetRegistry(); }
    void Update() { ECS::Systems::ScriptSystem::Update(Registry(), context, {}); }

    void Attach(const ECS::EntityID id, Hook update = {}, Hook destroy = {}, Hook exit = {}, const size_t index = 0) {
        auto *script = Registry().GetComponent<ECS::Components::ScriptComponent>(id);
        if (!script) {
            script = &Registry().AddComponent<ECS::Components::ScriptComponent>(id);
        }
        script->slots.resize(std::max(script->slots.size(), index + 1));
        auto &slot = script->slots[index];
        slot.isInitialized = true;
        slot.scriptPath = "native-fixture.obsl";
        for (size_t w = 0; w < runtime.worker_count(); ++w) {
            auto &interpreter = runtime.get_worker(w)->GetInterpreter();
            slot.instance_envs.push_back(runtime.get_worker(w)->copy_globals());
            interpreter.register_environment(slot.instance_envs.back());
            const auto bind = [&](const Hook &body, const char *name) -> ObSL::ObSLCallable * {
                if (!body) {
                    return nullptr;
                }
                auto *function = interpreter.gc.allocate<ObSL::NativeFunction>(1, body, name);
                interpreter.gc.add_root(function);
                return function;
            };
            slot.on_update_functions.push_back(bind(update, "on_update"));
            slot.on_destroy_functions.push_back(bind(destroy, "on_destroy"));
            slot.on_exit_functions.push_back(bind(exit, "on_exit"));
        }
    }

    void DestroyById(ObSL::Interpreter *interpreter, const ECS::EntityID id) {
        auto *function = std::get<ObSL::ObSLCallable *>(interpreter->get_global_environment()->get("DestroyEntity"));
        function->call(interpreter, {static_cast<double>(id)}, CallToken);
    }

    template <typename F> void DefineNative(const std::string &name, F body) {
        for (size_t w = 0; w < runtime.worker_count(); ++w) {
            runtime.get_worker(w)->GetInterpreter().define_native(name, body);
        }
    }
};

// This must return even when scripting is enabled and no entities are tagged.
TEST_P(ScriptLifecycleTests, EmptyDestructionQueueReturnsWithoutUpdatingScripts) {
    const auto id = Registry().CreateEntity();
    std::atomic<int> updates{0};
    Attach(id, [&](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { ++updates; return std::monostate{}; });
    ECS::Systems::ScriptSystem::ProcessPendingDestruction(Registry(), context, {});
    EXPECT_TRUE(Registry().IsValid(id));
    EXPECT_EQ(updates, 0);
}

TEST_P(ScriptLifecycleTests, DestroyBindingsDeferDeletionAndCallEachSlotOnce) {
    const auto parent = Registry().CreateEntity();
    const auto child = Registry().CreateEntity();
    Registry().SetParentDirect(child, parent);
    std::atomic<int> updates{0};
    std::atomic<int> destroys{0};
    const Hook onDestroy = [&](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { ++destroys; return std::monostate{}; };
    Attach(parent, [&](ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &) -> ObSL::Value {
        ++updates;
        DestroyById(interpreter, parent);
        auto *object = Scripting::CreateEntityObject(interpreter, Registry(), parent);
        std::get<ObSL::ObSLCallable *>(object->fields.at("Destroy"))->call(interpreter, {}, CallToken);
        return std::monostate{};
    }, onDestroy);
    Attach(child, [&](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { ++updates; return std::monostate{}; }, onDestroy);
    Attach(child, {}, onDestroy, {}, 1);
    Update();
    EXPECT_EQ(updates, 2);
    EXPECT_EQ(destroys, 0);
    ASSERT_TRUE(Registry().IsValid(parent));
    ASSERT_TRUE(Registry().IsValid(child));
    EXPECT_TRUE(Registry().HasComponent<ECS::Components::DestroyTagComponent>(child));
    Update();
    EXPECT_EQ(updates, 2);
    EXPECT_EQ(destroys, 3);
    EXPECT_FALSE(Registry().IsValid(parent));
    EXPECT_FALSE(Registry().IsValid(child));
    Update();
    EXPECT_EQ(destroys, 3);
}

TEST_P(ScriptLifecycleTests, DestroyBindingWithoutFrameContextStillRequestsDeletion) {
    const auto id = Registry().CreateEntity();
    std::atomic<int> destroys{0};
    Attach(id, {}, [&](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { ++destroys; return std::monostate{}; });
    DestroyById(&runtime.get_worker(0)->GetInterpreter(), id);
    ASSERT_TRUE(Registry().IsValid(id));
    EXPECT_TRUE(Registry().HasComponent<ECS::Components::DestroyTagComponent>(id));
    Update();
    EXPECT_FALSE(Registry().IsValid(id));
    EXPECT_EQ(destroys, 1);
}

TEST_P(ScriptLifecycleTests, ScriptlessSubtreesAreRemovedAcrossRepeatedRequests) {
    for (int cycle = 0; cycle < 200; ++cycle) {
        const auto root = Registry().CreateEntity();
        const auto child = Registry().CreateEntity();
        Registry().SetParentDirect(child, root);
        Registry().AddComponent<ECS::Components::DestroyTagComponent>(root);
        Registry().RequestDestroy(root);
        Registry().RequestDestroy(root);
        Update();
        ASSERT_FALSE(Registry().IsValid(root));
        ASSERT_FALSE(Registry().IsValid(child));
    }
    EXPECT_TRUE(Registry().GetLivingEntities().empty());
}

TEST_P(ScriptLifecycleTests, CallbackCommandsAndPackagedStringsSurviveUntilDeletion) {
    const auto id = Registry().CreateEntity();
    std::atomic<int> writes{0};
    std::atomic<int> invalidAccess{0};
    ECS::Systems::ScriptSystem::s_PackagedStringPools.try_emplace(std::pair{id, size_t{0}});
    Attach(id, {}, [&](ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &) -> ObSL::Value {
        auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
        worker->frame_context<Scripting::ScriptCommandBuffer>()->push([&](ECS::Registry &registry) {
            if (!registry.IsValid(id) || !ECS::Systems::ScriptSystem::s_PackagedStringPools.contains({id, 0})) {
                ++invalidAccess;
            }
            ++writes;
        });
        return std::monostate{};
    });
    Registry().RequestDestroy(id);
    Update();
    EXPECT_EQ(writes, 1);
    EXPECT_EQ(invalidAccess, 0);
    EXPECT_FALSE(Registry().IsValid(id));
    EXPECT_FALSE(ECS::Systems::ScriptSystem::s_PackagedStringPools.contains({id, 0}));
}

TEST_P(ScriptLifecycleTests, ChildCreatedByDestroyCallbackGetsItsOwnCallback) {
    const auto parent = Registry().CreateEntity();
    ECS::EntityID child = ECS::INVALID_ENTITY_ID;
    std::atomic<int> childDestroys{0};
    Attach(parent, {}, [&](ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &) -> ObSL::Value {
        auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
        worker->frame_context<Scripting::ScriptCommandBuffer>()->push([&](ECS::Registry &registry) {
            child = registry.CreateEntity();
            registry.SetParentDirect(child, parent);
            Attach(child, {}, [&](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { ++childDestroys; return std::monostate{}; });
        });
        return std::monostate{};
    });
    Registry().RequestDestroy(parent);
    Update();
    EXPECT_FALSE(Registry().IsValid(parent));
    ASSERT_TRUE(Registry().IsValid(child));
    EXPECT_EQ(childDestroys, 0);
    EXPECT_TRUE(Registry().HasComponent<ECS::Components::DestroyTagComponent>(child));
    Update();
    EXPECT_FALSE(Registry().IsValid(child));
    EXPECT_EQ(childDestroys, 1);
}

TEST_P(ScriptLifecycleTests, SceneExitDrainsTaggedEntitiesAndKeepsOrdinaryExitDistinct) {
    const auto parent = Registry().CreateEntity();
    const auto child = Registry().CreateEntity();
    const auto survivor = Registry().CreateEntity();
    Registry().SetParentDirect(child, parent);
    std::atomic<int> updates{0};
    std::atomic<int> destroys{0};
    std::atomic<int> taggedExits{0};
    std::atomic<int> survivorExits{0};
    const Hook update = [&](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { ++updates; return std::monostate{}; };
    const Hook destroy = [&](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { ++destroys; return std::monostate{}; };
    const Hook taggedExit = [&](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { ++taggedExits; return std::monostate{}; };
    Attach(parent, update, destroy, taggedExit);
    Attach(child, update, destroy, taggedExit);
    Attach(survivor, update, destroy, [&](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { ++survivorExits; return std::monostate{}; });
    Registry().AddComponent<ECS::Components::DestroyTagComponent>(parent);
    scene->OnExit();
    EXPECT_EQ(updates, 0);
    EXPECT_EQ(destroys, 2);
    EXPECT_EQ(taggedExits, 0);
    EXPECT_EQ(survivorExits, 1);
    EXPECT_TRUE(Registry().IsValid(survivor));
    EXPECT_FALSE(Registry().IsValid(parent));
    EXPECT_FALSE(Registry().IsValid(child));
    EXPECT_EQ(context.uiSystem, nullptr);
    EXPECT_EQ(context.uiCmdBuf, nullptr);
}

TEST_P(ScriptLifecycleTests, SceneExitProcessesDestructionRequestedByExitHooks) {
    const auto id = Registry().CreateEntity();
    std::atomic<int> exits{0};
    std::atomic<int> destroys{0};
    std::atomic<int> invalidUi{0};
    Attach(id, {}, [&](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
        if (context.uiSystem != &scene->GetUISystem()) {
            ++invalidUi;
        }
        ++destroys;
        return std::monostate{};
    }, [&](ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &) -> ObSL::Value {
        ++exits;
        DestroyById(interpreter, id);
        return std::monostate{};
    });
    scene->OnExit();
    EXPECT_EQ(exits, 1);
    EXPECT_EQ(destroys, 1);
    EXPECT_EQ(invalidUi, 0);
    EXPECT_FALSE(Registry().IsValid(id));
}

TEST_P(ScriptLifecycleTests, SceneExitDrainsChildrenCreatedDuringDestruction) {
    const auto parent = Registry().CreateEntity();
    ECS::EntityID child = ECS::INVALID_ENTITY_ID;
    std::atomic<int> destroys{0};
    Attach(parent, {}, [&](ObSL::Interpreter *interpreter, const std::vector<ObSL::Value> &) -> ObSL::Value {
        auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
        worker->frame_context<Scripting::ScriptCommandBuffer>()->push([&](ECS::Registry &registry) {
            child = registry.CreateEntity();
            registry.SetParentDirect(child, parent);
            Attach(child, {}, [&](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { ++destroys; return std::monostate{}; });
        });
        return std::monostate{};
    });
    Registry().RequestDestroy(parent);
    scene->OnExit();
    EXPECT_FALSE(Registry().IsValid(parent));
    EXPECT_FALSE(Registry().IsValid(child));
    EXPECT_EQ(destroys, 1);
}

TEST_P(ScriptLifecycleTests, ScriptSlotReferencesSurviveComponentPoolGrowthDuringInitialization) {
    std::atomic<int> updates{0};
    DefineNative("CountUpdate", [&]() { ++updates; });
    DefineNative("GrowScriptPool", [&]() {
        for (int i = 0; i < 256; ++i) {
            Attach(Registry().CreateEntity());
        }
    });
    ASSERT_NO_FATAL_FAILURE(Write("assets/scripts/grow.obsl", "GrowScriptPool(); fn on_update(dt) { CountUpdate(); }"));
    const auto id = Registry().CreateEntity();
    Registry().AddComponent<ECS::Components::ScriptComponent>(id).slots.emplace_back().scriptPath = "assets/scripts/grow.obsl";
    Update();
    ASSERT_TRUE(Registry().GetComponent<ECS::Components::ScriptComponent>(id)->slots[0].isInitialized);
    EXPECT_EQ(Registry().GetLivingEntities().size(), 257u);
    EXPECT_EQ(updates, 1);
    Update();
    EXPECT_EQ(updates, 2);
}

TEST_P(ScriptLifecycleTests, HotReloadTargetsChangedSlotAndReleasesOldDestroyRoots) {
    std::atomic<int> firstCalls{0};
    std::atomic<int> oldSecondCalls{0};
    std::atomic<int> newSecondCalls{0};
    DefineNative("First", [&]() { ++firstCalls; });
    DefineNative("OldSecond", [&]() { ++oldSecondCalls; });
    DefineNative("NewSecond", [&]() { ++newSecondCalls; });
    ASSERT_NO_FATAL_FAILURE(Write("assets/scripts/first.obsl", "fn on_update(dt) { First(); }"));
    ASSERT_NO_FATAL_FAILURE(Write("assets/scripts/second.obsl", "fn on_update(dt) { OldSecond(); }"));
    const auto id = Registry().CreateEntity();
    auto &script = Registry().AddComponent<ECS::Components::ScriptComponent>(id);
    script.slots.resize(2);
    script.slots[0].scriptPath = "assets/scripts/first.obsl";
    script.slots[1].scriptPath = "assets/scripts/second.obsl";
    Update();
    ASSERT_TRUE(script.slots[1].isInitialized);
    for (size_t w = 0; w < runtime.worker_count(); ++w) {
        auto &interpreter = runtime.get_worker(w)->GetInterpreter();
        auto *hook = interpreter.gc.allocate<TrackedHook>(&releasedHooks);
        interpreter.gc.add_root(hook);
        script.slots[1].on_destroy_functions[w] = hook;
    }
    const auto oldTime = script.slots[1].lastModified;
    ASSERT_NO_FATAL_FAILURE(Write("assets/scripts/second.obsl", "fn on_update(dt) { NewSecond(); }"));
    std::filesystem::last_write_time(directory / "assets/scripts/second.obsl", oldTime + std::chrono::seconds{2});
    context.frameCount = 300;
    Update();
    EXPECT_EQ(firstCalls, 2);
    EXPECT_EQ(oldSecondCalls, 1);
    EXPECT_EQ(newSecondCalls, 1);
    for (size_t w = 0; w < runtime.worker_count(); ++w) {
        auto &interpreter = runtime.get_worker(w)->GetInterpreter();
        interpreter.set_current_environment(interpreter.get_global_environment());
        interpreter.gc.collect();
    }
    EXPECT_EQ(releasedHooks, static_cast<int>(runtime.worker_count()));
}

TEST_P(ScriptLifecycleTests, PackagedAstRetainsStringsUntilDestroyCallbackCompletes) {
    const std::string source = "fn on_destroy(dt) { RecordDestroyed(\"packaged string survives\"); }";
    ObSL::Lexer lexer(source);
    ObSL::Parser parser(lexer.tokenize());
    const auto statements = parser.parse();
    ObSL::ASTSerializer serializer;
    IO::ContainerWriter writer;
    writer.add_compiled_script("assets/scripts/destroy.obsl", serializer.finalize(statements), true);
    writer.write(directory / "data.obpak");
    IO::VFS::MountPackage(directory / "data.obpak");
    std::atomic<int> callbacks{0};
    std::atomic<int> badStrings{0};
    DefineNative("RecordDestroyed", [&](const std::string &value) {
        if (value != "packaged string survives") {
            ++badStrings;
        }
        ++callbacks;
    });
    const auto id = Registry().CreateEntity();
    Registry().AddComponent<ECS::Components::ScriptComponent>(id).slots.emplace_back().scriptPath = "assets/scripts/destroy.obsl";
    Update();
    ASSERT_TRUE(Registry().GetComponent<ECS::Components::ScriptComponent>(id)->slots[0].isInitialized);
    ASSERT_TRUE(ECS::Systems::ScriptSystem::s_PackagedStringPools.contains({id, 0}));
    Registry().RequestDestroy(id);
    Update();
    EXPECT_EQ(callbacks, 1);
    EXPECT_EQ(badStrings, 0);
    EXPECT_FALSE(ECS::Systems::ScriptSystem::s_PackagedStringPools.contains({id, 0}));
}

INSTANTIATE_TEST_SUITE_P(WorkerCounts, ScriptLifecycleTests, testing::Values(size_t{1}, size_t{2}),
    [](const testing::TestParamInfo<size_t> &info) { return "Workers" + std::to_string(info.param); });
