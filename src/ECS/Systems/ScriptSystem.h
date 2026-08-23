#pragma once

#include <filesystem>
#include <vector>
#include <map>
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
            const size_t old_count = slot.on_update_functions.size();
            for (size_t w = 0; w < old_count; ++w) {
                if (auto *func = slot.on_update_functions[w])
                    runtime.get_worker(w)->GetInterpreter().gc.remove_root(func);
                if (auto *func = slot.on_destroy_functions[w])
                    runtime.get_worker(w)->GetInterpreter().gc.remove_root(func);
                if (auto *func = slot.on_exit_functions[w])
                    runtime.get_worker(w)->GetInterpreter().gc.remove_root(func);
            }

            slot.instance_envs.resize(num_workers);
            slot.on_update_functions.assign(num_workers, nullptr);
            slot.on_destroy_functions.assign(num_workers, nullptr);
            slot.on_exit_functions.assign(num_workers, nullptr);

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
        constexpr ObSL::Token call_token{ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0};
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
                s_AllSlots.push_back({raw_id, script, i});

                if (!slot.isInitialized) {
                    pendingInits.push_back({raw_id, script, i, false});
                }

                try {
                    if (shouldPollReload) {
                        if (slot.resolvedPath.empty() && !slot.scriptPath.empty()) {
                            slot.resolvedPath = IO::VFS::Resolve(slot.scriptPath);
                        }
                        if (const std::filesystem::path &resolvedPath = slot.resolvedPath; std::filesystem::exists(resolvedPath)) {
                            if (const auto current_time = std::filesystem::last_write_time(resolvedPath); current_time > slot.lastModified && slot.isInitialized) {
                                pendingInits.push_back({raw_id, script, i, true});
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
                s_Buckets[w].push_back({slot.on_update_functions[w], slot.instance_envs[w], slot.scriptPath, "on_update"});
                ++totalWork;
            }
        }

        registry.ForEach<Components::DestroyTagComponent>([&](const Entity entity, Components::DestroyTagComponent *) {
            const auto entity_id = static_cast<EntityID>(entity);
            if (const auto script = registry.GetComponent<Components::ScriptComponent>(entity_id)) {
                for (size_t i = 0; i < script->slots.size(); i++) {
                    auto &slot = script->slots[i];
                    s_PackagedStringPools.erase({entity_id, i});
                    if (slot.isInitialized && !slot.on_destroy_functions.empty() && slot.on_destroy_functions[0]) {
                        const size_t w = (entity_id + i) % num_workers;
                        s_Buckets[w].push_back({slot.on_destroy_functions[w], slot.instance_envs[w], slot.scriptPath, "on_destroy"});
                        ++totalWork;
                    }
                }
            }
        });

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
        constexpr ObSL::Token call_token{ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0};

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
                    buckets[w].push_back({slot.on_exit_functions[w], slot.instance_envs[w], slot.scriptPath});
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

} // namespace ECS::Systems::ScriptSystem
