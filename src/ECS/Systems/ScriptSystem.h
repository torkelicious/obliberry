#pragma once

#include <cstddef>
#include <filesystem>
#include <variant>
#include <vector>
#include <map>
#include "ECS/Systems/Collision/CollisionWorld.h"
#include "ECS/Types.h"
#include "Logger/LoggerService.h"
#include "Core/EngineContext.h"
#include "ECS/Registry.h"
#include "ECS/Components/DestroyTagComponent.h"
#include "ECS/Components/ScriptComponent.h"
#include "IO/VFS/VFS.h"
#include <ObSL/Lexer.h>
#include <ObSL/ScriptRuntime.h>
#include <ObSL/ScriptWorker.h>
#include <ObSL/Parser.h>
#include <ObSL/ASTDeserializer.h>
#include "ObSL/Parser/ast.h"
#include "Scripting/EngineLib/ScriptCommandBuffer.h"
#include "Scripting/EngineLib/EntityWrapperCache.h"
#include "Scripting/EngineLib/EngineLibFactories.h"
#include "Platform/Threading/ThreadPool.h"

namespace ECS::Systems::ScriptSystem {

    // map tied to the AST instance :  ( EntityID , script slot index )
    inline std::map<std::pair<EntityID, size_t>, decltype(ObSL::ASTDeserializer::deserialize(std::vector<uint8_t>()))> s_PackagedStringPools;

    inline void SetupScriptRuntime(ObSL::ScriptRuntime &runtime) {
        for (size_t w = 0; w < runtime.worker_count(); ++w) {
            runtime.get_worker(w)->GetInterpreter().set_module_loader([](const std::string &path) -> std::optional<ObSL::ModuleResult> {
                std::string_view dataView;
                std::string ownedData;
                if (const auto view = IO::VFS::ReadVirtualView(path)) {
                    dataView = *view;
                } else if (auto owned = IO::VFS::ReadVirtual(path)) {
                    ownedData = std::move(*owned);
                    dataView = ownedData;
                } else {
                    return std::nullopt;
                }

                ObSL::ModuleResult result;
                if (IO::VFS::IsPackaged()) {
                    const std::vector<uint8_t> blob(dataView.begin(), dataView.end());
                    result.kind = ObSL::ModuleResult::Kind::PrecompiledAst;
                    result.ast_module = ObSL::ASTDeserializer::deserialize(blob);
                } else {
                    result.kind = ObSL::ModuleResult::Kind::Source;
                    result.source = std::string(dataView);
                }
                return result;
            });
        }
    }

    inline void InitializeScript(Registry &registry, const EntityID entityId, Components::ScriptComponent *script, ObSL::ScriptRuntime &runtime, const size_t scriptIndex = 0) {
        if (scriptIndex >= script->slots.size() || script->slots[scriptIndex].isInitialized)
            return;

        auto &slot = script->slots[scriptIndex];

        auto fileData = IO::VFS::ReadVirtual(slot.scriptPath);
        if (!fileData.has_value()) {
            if (auto *logger = Logging::LoggerService::Get()) {
                logger->log("ScriptSystem", "Failed to open script via VFS: " + slot.scriptPath, Logging::LogSeverity::Error);
            }
            return;
        }

        try {
            const size_t num_workers = runtime.worker_count();

            // Packaged AST
            if (IO::VFS::IsPackaged()) {
                const std::vector<uint8_t> binary_blob(fileData->begin(), fileData->end());
                try {
                    // replaces old entry
                    auto &deserialized = s_PackagedStringPools[{entityId, scriptIndex}];
                    deserialized = ObSL::ASTDeserializer::deserialize(binary_blob);

                    auto &[string_pool, statements] = deserialized;

                    slot.ast_nodes = std::move(statements);
                    slot.lastModified = std::filesystem::file_time_type::min();
                } catch (const std::exception &e) {
                    if (auto *logger = Logging::LoggerService::Get()) {
                        logger->log("ScriptSystem", "AST Deserialization failed for: " + slot.scriptPath + " Err: " + e.what(), Logging::LogSeverity::Error);
                    }
                    return;
                }
            } else {
                // Loose file source code
                slot.source_code = std::move(fileData.value());

                slot.resolvedPath = IO::VFS::Resolve(slot.scriptPath);
                if (const std::filesystem::path &resolvedPath = slot.resolvedPath; std::filesystem::exists(resolvedPath)) {
                    slot.lastModified = std::filesystem::last_write_time(resolvedPath);
                }

                // Parse once & share
                ObSL::Lexer lexer(slot.source_code);
                const std::vector<ObSL::Token> tokens = lexer.tokenize();

                ObSL::Parser parser(tokens);
                slot.ast_nodes = std::move(parser.parse());
            }

            // Remove old GC roots before re init
            auto remove_roots = [&](auto &functions) {
                for (size_t w = 0; w < functions.size(); ++w) {
                    if (functions[w]) {
                        runtime.get_worker(w)->GetInterpreter().gc.remove_root(functions[w]);
                    }
                }
            };

            {
                std::vector<std::vector<ObSL::ObSLCallable *>> hooks = {slot.on_update_functions,          slot.on_exit_functions,           slot.on_exit_functions,
                                                                        slot.on_collision_enter_functions, slot.on_collision_stay_functions, slot.on_collision_exit_functions,
                                                                        slot.on_trigger_enter_functions,   slot.on_trigger_stay_functions,   slot.on_trigger_exit_functions};

                for (const auto &hook : hooks) {
                    remove_roots(hook);
                }
            }


            slot.instance_envs.resize(num_workers);
            // hooks
            // basic hooks
            slot.on_update_functions.assign(num_workers, nullptr);
            slot.on_destroy_functions.assign(num_workers, nullptr);
            slot.on_exit_functions.assign(num_workers, nullptr);
            // collider hooks
            slot.on_collision_enter_functions.assign(num_workers, nullptr);
            slot.on_collision_stay_functions.assign(num_workers, nullptr);
            slot.on_collision_exit_functions.assign(num_workers, nullptr);

            slot.on_trigger_enter_functions.assign(num_workers, nullptr);
            slot.on_trigger_stay_functions.assign(num_workers, nullptr);
            slot.on_trigger_exit_functions.assign(num_workers, nullptr);


            // per worker env with entity wrappers
            for (size_t w = 0; w < num_workers; ++w) {
                auto *worker = runtime.get_worker(w);
                auto &interp = worker->GetInterpreter();

                slot.instance_envs[w] = worker->copy_globals();
                interp.register_environment(slot.instance_envs[w]);

                auto *entityWrapper = Scripting::CreateEntityObject(&interp, registry, entityId);
                slot.instance_envs[w]->define("this", entityWrapper);
            }

            // run top level code once
            runtime.get_worker(0)->execute(slot.ast_nodes, slot.instance_envs[0]);

            // Bind hook functions
            auto bind_hook = [&](const char *name, std::vector<ObSL::ObSLCallable *> &target) {
                try {
                    const auto val = slot.instance_envs[0]->get(name);
                    if (!std::holds_alternative<ObSL::ObSLCallable *>(val))
                        return;
                    auto *base_func = std::get<ObSL::ObSLCallable *>(val);

                    if (auto *obsl_func = dynamic_cast<ObSL::ObSLFunction *>(base_func)) {
                        for (size_t w = 0; w < num_workers; ++w) {
                            auto *worker_w = runtime.get_worker(w);
                            auto &interp_w = worker_w->GetInterpreter();
                            auto this_val = slot.instance_envs[w]->get("this");
                            auto *entity_w = std::get<ObSL::ObSLObject *>(this_val);
                            auto *bound = obsl_func->bind(entity_w, &interp_w);
                            target[w] = bound;
                            interp_w.gc.add_root(bound);
                        }
                    } else {
                        for (size_t w = 0; w < num_workers; ++w) {
                            target[w] = base_func;
                            runtime.get_worker(w)->GetInterpreter().gc.add_root(base_func);
                        }
                    }
                } catch (...) {
                }
            };

            bind_hook("on_update", slot.on_update_functions);
            bind_hook("on_destroy", slot.on_destroy_functions);
            bind_hook("on_exit", slot.on_exit_functions);

            bind_hook("on_collision_enter", slot.on_collision_enter_functions);
            bind_hook("on_collision_stay", slot.on_collision_stay_functions);
            bind_hook("on_collision_exit", slot.on_collision_exit_functions);

            bind_hook("on_trigger_enter", slot.on_trigger_enter_functions);
            bind_hook("on_trigger_stay", slot.on_trigger_stay_functions);
            bind_hook("on_trigger_exit", slot.on_trigger_exit_functions);


            slot.isInitialized = true;
            if (auto *logger = Logging::LoggerService::Get()) {
                logger->log("ScriptSystem", "Initialized '" + slot.scriptPath + "' across " + std::to_string(num_workers) + " worker(s)", Logging::LogSeverity::Info);
            }
        } catch (const std::exception &e) {
            if (auto *logger = Logging::LoggerService::Get()) {
                logger->log("ScriptSystem", "Error compiling/executing '" + slot.scriptPath + "':\n  " + e.what(), Logging::LogSeverity::Error);
            }
        }
    }

    inline void Update(Registry &registry, const Core::EngineContext &ctx) {
        if (!ctx.scriptPool || !ctx.threadPool)
            return;
        constexpr ObSL::Token call_token{.type = ObSL::TokenType::LEFT_PAREN, .lexeme = "(", .line = 0, .column = 0, .start_pos = 0, .end_pos = 0};
        constexpr uint64_t kReloadPollIntervalFrames = 300;
        const bool shouldPollReload = !IO::VFS::IsPackaged() && ctx.frameCount % kReloadPollIntervalFrames == 0;

        static Scripting::ScriptCommandBuffer cmd_buf;
        const size_t num_workers = ctx.scriptPool->worker_count();

        // make sure command buffer / frame context before code runs
        for (size_t w = 0; w < num_workers; ++w)
            ctx.scriptPool->get_worker(w)->set_frame_context(&cmd_buf);

        struct PendingScriptInit {
            EntityID entityId;
            Components::ScriptComponent *script;
            size_t scriptIndex;
            bool isReload;
        };
        std::vector<PendingScriptInit> pendingInits;

        // it is captured once and reused
        struct ScriptSlotRef {
            EntityID entityId;
            Components::ScriptComponent *script;
            size_t slotIndex;
        };
        static std::vector<ScriptSlotRef> s_AllSlots;
        s_AllSlots.clear();

        registry.ForEach<Components::ScriptComponent>([&](const Entity entity, Components::ScriptComponent *script) {
            const auto raw_id = static_cast<EntityID>(entity);
            for (size_t i = 0; i < script->slots.size(); i++) {
                auto &slot = script->slots[i];
                s_AllSlots.push_back({.entityId = raw_id, .script = script, .slotIndex = i});

                if (!slot.isInitialized) {
                    pendingInits.push_back({.entityId = raw_id, .script = script, .scriptIndex = i, .isReload = false});
                }

                try {
                    if (shouldPollReload) {
                        if (slot.resolvedPath.empty() && !slot.scriptPath.empty()) {
                            slot.resolvedPath = IO::VFS::Resolve(slot.scriptPath);
                        }
                        if (const std::filesystem::path &resolvedPath = slot.resolvedPath; std::filesystem::exists(resolvedPath)) {
                            if (const auto current_time = std::filesystem::last_write_time(resolvedPath); current_time > slot.lastModified && slot.isInitialized) {
                                pendingInits.push_back({.entityId = raw_id, .script = script, .scriptIndex = i, .isReload = true});
                            }
                        }
                    }
                } catch (const std::exception &e) {
                    if (auto *logger = Logging::LoggerService::Get()) {
                        logger->log("ScriptSystem", "reload error: " + std::string(e.what()), Logging::LogSeverity::Error);
                    }
                }
            }
        });

        // initialize scripts outside the ForEach iteration
        for (const auto &entry : pendingInits) {
            if (!registry.IsValid(entry.entityId))
                continue;
            auto *script = registry.GetComponent<Components::ScriptComponent>(entry.entityId);
            if (!script)
                continue;
            if (entry.scriptIndex >= script->slots.size())
                continue;
            if (!script->slots[entry.scriptIndex].isInitialized) {
                InitializeScript(registry, entry.entityId, script, *ctx.scriptPool, entry.scriptIndex);
            } else if (entry.isReload) {
                script->slots[entry.scriptIndex].isInitialized = false;
                InitializeScript(registry, entry.entityId, script, *ctx.scriptPool, entry.scriptIndex);
                if (auto *logger = Logging::LoggerService::Get()) {
                    logger->log("ScriptSystem", "Hot-reloaded script: " + script->slots[entry.scriptIndex].scriptPath, Logging::LogSeverity::Info);
                }
            }
        }

        struct UpdateWork {
            ObSL::ObSLCallable *func;
            std::shared_ptr<ObSL::Environment> env;
            std::string_view scriptPath;
            const char *hookName;
        };
        // persistent across frames
        static std::vector<std::vector<UpdateWork>> s_Buckets;
        s_Buckets.resize(num_workers);
        for (auto &b : s_Buckets)
            b.clear();
        size_t totalWork = 0;

        for (const auto &ref : s_AllSlots) {
            if (!registry.IsValid(ref.entityId)) // do not assume even though i prolly can?
                continue;
            auto &slot = ref.script->slots[ref.slotIndex];
            if (slot.isInitialized && !slot.on_update_functions.empty() && slot.on_update_functions[0]) {
                const size_t w = (ref.entityId + ref.slotIndex) % num_workers;
                s_Buckets[w].push_back({.func = slot.on_update_functions[w], .env = slot.instance_envs[w], .scriptPath = slot.scriptPath, .hookName = "on_update"});
                ++totalWork;
            }
        }

        std::vector<EntityID> destroyTagged;
        registry.ForEach<Components::DestroyTagComponent>([&](const Entity entity, Components::DestroyTagComponent *) {
            const auto entity_id = static_cast<EntityID>(entity);
            if (const auto script = registry.GetComponent<Components::ScriptComponent>(entity_id)) {
                for (size_t i = 0; i < script->slots.size(); i++) {
                    auto &slot = script->slots[i];
                    s_PackagedStringPools.erase({entity_id, i});
                    if (slot.isInitialized && !slot.on_destroy_functions.empty() && slot.on_destroy_functions[0]) {
                        const size_t w = (entity_id + i) % num_workers;
                        s_Buckets[w].push_back({.func = slot.on_destroy_functions[w], .env = slot.instance_envs[w], .scriptPath = slot.scriptPath, .hookName = "on_destroy"});
                        ++totalWork;
                    }
                }
            }
            destroyTagged.push_back(entity_id);
        });

        for (const EntityID id : destroyTagged)
            registry.RemoveComponent<Components::DestroyTagComponent>(id);

        // Static reuse of arguments
        static std::vector<ObSL::Value> args(1);
        args[0] = static_cast<double>(ctx.deltaTime);

        Platform::Threading::TaskGroup scriptGroup;

        for (size_t w = 0; w < num_workers; ++w) {
            if (s_Buckets[w].empty())
                continue;

            scriptGroup.Add();
            ctx.threadPool->enqueue(static_cast<Platform::Threading::Task>([&ctx, w, &call_token, &scriptGroup] {
                auto *worker = ctx.scriptPool->get_worker(w);
                auto &interp = worker->GetInterpreter();
                for (auto &[func, env, scriptPath, hookName] : s_Buckets[w]) {
                    try {
                        if (func && env) {
                            interp.set_current_environment(env);
                            func->call(&interp, args, call_token);
                        }
                    } catch (const std::exception &e) {
                        if (auto *logger = Logging::LoggerService::Get()) {
                            logger->log("ScriptSystem", "Exception in " + std::string(hookName) + " (" + std::string(scriptPath) + ") : " + e.what(), Logging::LogSeverity::Error);
                        }
                    } catch (...) {
                        if (auto *logger = Logging::LoggerService::Get()) {
                            logger->log("ScriptSystem", "Unknown Exception in " + std::string(hookName) + " (" + std::string(scriptPath) + ")", Logging::LogSeverity::Error);
                        }
                    }
                }
                scriptGroup.Done();
            }));
        }

        if (totalWork > 0)
            scriptGroup.Wait();

        // flush deferred registry writes
        cmd_buf.flush(registry);

        for (size_t w = 0; w < num_workers; ++w)
            ctx.scriptPool->get_worker(w)->clear_frame_context();
    }

    inline void OnSceneExit(Registry &registry, const Core::EngineContext &ctx) {
        if (!ctx.scriptPool || !ctx.threadPool)
            return;
        constexpr ObSL::Token call_token{.type = ObSL::TokenType::LEFT_PAREN, .lexeme = "(", .line = 0, .column = 0, .start_pos = 0, .end_pos = 0};

        // Static reuse
        static Scripting::ScriptCommandBuffer cmd_buf;
        const size_t num_workers = ctx.scriptPool->worker_count();

        for (size_t w = 0; w < num_workers; ++w)
            ctx.scriptPool->get_worker(w)->set_frame_context(&cmd_buf);

        size_t active_exits = 0;
        registry.ForEach<Components::ScriptComponent>([&](const Entity, const Components::ScriptComponent *script) {
            for (const auto &slot : script->slots) {
                if (slot.isInitialized && !slot.on_exit_functions.empty() && slot.on_exit_functions[0])
                    ++active_exits;
            }
        });
        const size_t exit_workers = std::min(num_workers, std::max(size_t{1}, active_exits));

        // collect on_exit stuff
        struct ExitWork {
            ObSL::ObSLCallable *func;
            std::shared_ptr<ObSL::Environment> env;
            std::string_view scriptPath;
        };
        std::vector<std::vector<ExitWork>> buckets(exit_workers);

        registry.ForEach<Components::ScriptComponent>([&](const Entity entity, const Components::ScriptComponent *script) {
            const auto entity_id = static_cast<EntityID>(entity);
            for (size_t i = 0; i < script->slots.size(); i++) {
                auto &slot = script->slots[i];
                if (slot.isInitialized && !slot.on_exit_functions.empty() && slot.on_exit_functions[0]) {
                    const size_t w = (entity_id + i) % exit_workers;
                    buckets[w].push_back({.func = slot.on_exit_functions[w], .env = slot.instance_envs[w], .scriptPath = slot.scriptPath});
                }
            }
        });

        const double dt = ctx.deltaTime;

        static std::vector<ObSL::Value> args(1);
        args[0] = static_cast<double>(dt);

        Platform::Threading::TaskGroup exitGroup;

        for (size_t w = 0; w < exit_workers; ++w) {
            if (buckets[w].empty())
                continue;

            exitGroup.Add();
            ctx.threadPool->enqueue(static_cast<Platform::Threading::Task>([&buckets, &ctx, w, &call_token, &exitGroup] {
                auto *worker = ctx.scriptPool->get_worker(w);
                auto &interp = worker->GetInterpreter();
                for (auto &[func, env, scriptPath] : buckets[w]) {
                    try {
                        if (func && env) {
                            interp.set_current_environment(env);
                            func->call(&interp, args, call_token);
                        }
                    } catch (const std::exception &e) {
                        if (auto *logger = Logging::LoggerService::Get()) {
                            logger->log("ScriptSystem", "Exception in on_exit (" + std::string(scriptPath) + ") : " + e.what(), Logging::LogSeverity::Error);
                        }
                    } catch (...) {
                        if (auto *logger = Logging::LoggerService::Get()) {
                            logger->log("ScriptSystem", "Unknown Exception in on_exit (" + std::string(scriptPath) + ")", Logging::LogSeverity::Error);
                        }
                    }
                }
                exitGroup.Done();
            }));
        }

        if (active_exits > 0)
            exitGroup.Wait();

        cmd_buf.flush(registry);
        s_PackagedStringPools.clear();
        Scripting::EntityWrapperCache::ClearAll();
        for (size_t w = 0; w < num_workers; ++w)
            ctx.scriptPool->get_worker(w)->clear_frame_context();
    }

    inline const std::vector<ObSL::ObSLCallable *> *GetCollisionHook(const Components::ScriptSlot &slot, Collision::CollisionEventType type, bool trigger) {
        using Type = Collision::CollisionEventType;
        switch (type) {
            case Type::Enter:
                return trigger ? &slot.on_trigger_enter_functions : &slot.on_collision_enter_functions;
            case Type::Stay:
                return trigger ? &slot.on_trigger_stay_functions : &slot.on_collision_enter_functions;
            case Type::Exit:
                return trigger ? &slot.on_trigger_exit_functions : &slot.on_collision_exit_functions;
        }
        return nullptr;
    }


    inline void DispatchCollisionEvents(Registry &registry, const Core::EngineContext &ctx, const std::vector<Collision::CollisionEvent> &events) {
        if (!ctx.scriptPool || events.empty()) {
            return;
        }

        const size_t workerCount = ctx.scriptPool->worker_count();

        if (workerCount == 0) {
            return;
        }

        struct CollisionWork {
            EntityID receiver;
            EntityID other;
            size_t slotIdx;
            Collision::CollisionEventType type;
            bool trigger;
        };

        std::vector<CollisionWork> work;

        auto qReceiver = [&](EntityID receiver, EntityID other, const Collision::CollisionEvent &event) {
            if (!registry.IsValid(receiver)) {
                return;
            }

            const auto *scripts = registry.GetComponent<Components::ScriptComponent>(receiver);
            if (!scripts) {
                return;
            }

            for (size_t slotIdx = 0; slotIdx < scripts->slots.size(); ++slotIdx) {
                const auto &slot = scripts->slots[slotIdx];
                if (!slot.isInitialized) {
                    continue;
                }

                const size_t workerIdx = (receiver + slotIdx) % workerCount;
                const auto *hooks = GetCollisionHook(slot, event.type, event.isTrigger);

                if (!hooks || workerIdx >= hooks->size() || !(*hooks)[workerIdx]) {
                    continue;
                }

                work.push_back({receiver, other, slotIdx, event.type, event.isTrigger});
            }
        };

        for (const auto &event : events) {
            qReceiver(event.entityA, event.entityB, event);
            qReceiver(event.entityB, event.entityA, event);
        }

        if (work.empty()) {
            return;
        }

        Scripting::ScriptCommandBuffer commands;

        for (size_t w = 0; w < workerCount; ++w) {
            ctx.scriptPool->get_worker(w)->set_frame_context(&commands);
        }

        struct ContextCleanup {
            const Core::EngineContext &ctx;
            size_t count;

            ~ContextCleanup() {
                for (size_t w = 0; w < count; ++w) {
                    ctx.scriptPool->get_worker(w)->clear_frame_context();
                }
            }
        };

        ContextCleanup cleanup{ctx, workerCount};

        constexpr ObSL::Token callToken{.type = ObSL::TokenType::LEFT_PAREN, .lexeme = "(", .line = 0, .column = 0, .start_pos = 0, .end_pos = 0};

        // refetch comp
        for (const CollisionWork &item : work) {
            if (!registry.IsValid(item.receiver)) {
                continue;
            }

            const auto *scripts = registry.GetComponent<Components::ScriptComponent>(item.receiver);

            if (!scripts || item.slotIdx >= scripts->slots.size()) {
                continue;
            }

            const auto &slot = scripts->slots[item.slotIdx];

            if (!slot.isInitialized) {
                continue;
            }

            const size_t workerIdx = (item.receiver + item.slotIdx) % workerCount;

            const auto *hooks = GetCollisionHook(slot, item.type, item.trigger);

            if (!hooks || workerIdx >= hooks->size() || workerIdx >= slot.instance_envs.size()) {
                continue;
            }

            // copy b4 executions
            auto *function = (*hooks)[workerIdx];
            auto env = slot.instance_envs[workerIdx];
            const std::string scriptPath = slot.scriptPath;

            if (!function || !env) {
                continue;
            }

            auto *worker = ctx.scriptPool->get_worker(workerIdx);
            auto &interpreter = worker->GetInterpreter();

            try {
                interpreter.set_current_environment(env);
                ObSL::Value otherArg = std::monostate{};

                if (registry.IsValid(item.other)) {
                    otherArg = Scripting::CreateEntityObject(&interpreter, registry, item.other);
                }
                Scripting::EngineLibFactories::GCProtectGuard protect(&interpreter, otherArg); // keep alive

                const std::vector<ObSL::Value> args{otherArg};

                function->call(&interpreter, args, callToken);
            } catch (const std::exception &error) {
                if (auto *logger = Logging::LoggerService::Get()) {
                    logger->log("ScriptSystem", "Collision callback failed (" + scriptPath + "): " + error.what(), Logging::LogSeverity::Error);
                }
            } catch (...) {
                if (auto *logger = Logging::LoggerService::Get()) {
                    logger->log("ScriptSystem", "Unknown collision callback error (" + scriptPath + ")", Logging::LogSeverity::Error);
                }
            }
        }
        commands.flush(registry);
    }


} // namespace ECS::Systems::ScriptSystem
