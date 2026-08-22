#pragma once

#include <filesystem>
#include <vector>
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
#include "Scripting/EngineLib/EngineLibFactories.h"
#include "Platform/Threading/ThreadPool.h"

namespace ECS::Systems::ScriptSystem {
    inline void InitializeScript(Registry &registry, const EntityID entityId, Components::ScriptComponent *script, ObSL::ScriptRuntime &runtime, const size_t scriptIndex = 0) {
        if (scriptIndex >= script->scriptPaths.size() || script->isInitialized[scriptIndex])
            return;

        if (script->resolvedScriptPaths.size() != script->scriptPaths.size()) {
            script->resolvedScriptPaths.resize(script->scriptPaths.size());
        }

        auto fileData = IO::VFS::ReadVirtual(script->scriptPaths[scriptIndex]);
        if (!fileData.has_value()) {
            if (auto *logger = Logging::LoggerService::Get()) {
                logger->log("ScriptSystem", "Failed to open script via VFS: " + script->scriptPaths[scriptIndex], Logging::LogSeverity::Error);
            }
            return;
        }

        try {
            // configure module loader
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

            // Packaged AST
            if (IO::VFS::IsPackaged()) {
                const std::vector<uint8_t> binary_blob(fileData->begin(), fileData->end());
                try {
                    // static container keeps the deserialized string_pools alive for the application lifetime to avoid
                    // pointing at dead mem nce goes oos
                    static std::vector<decltype(ObSL::ASTDeserializer::deserialize(std::vector<uint8_t>()))> s_PackagedStringPools;

                    auto &deserialized = s_PackagedStringPools.emplace_back(ObSL::ASTDeserializer::deserialize(binary_blob));
                    auto &[string_pool, statements] = deserialized;

                    script->ast_nodes[scriptIndex] = std::move(statements);
                    script->lastModified[scriptIndex] = std::filesystem::file_time_type::min();
                } catch (const std::exception &e) {
                    if (auto *logger = Logging::LoggerService::Get()) {
                        logger->log("ScriptSystem", "AST Deserialization failed for: " + script->scriptPaths[scriptIndex], Logging::LogSeverity::Error);
                    }
                    return;
                }
            } else {
                // Loose file source code
                script->source_codes[scriptIndex] = std::move(fileData.value());

                script->resolvedScriptPaths[scriptIndex] = IO::VFS::Resolve(script->scriptPaths[scriptIndex]);
                if (const std::filesystem::path &resolvedPath = script->resolvedScriptPaths[scriptIndex]; std::filesystem::exists(resolvedPath)) {
                    script->lastModified[scriptIndex] = std::filesystem::last_write_time(resolvedPath);
                }

                // Parse once & share
                ObSL::Lexer lexer(script->source_codes[scriptIndex]);
                const std::vector<ObSL::Token> tokens = lexer.tokenize();

                ObSL::Parser parser(tokens);
                script->ast_nodes[scriptIndex] = std::move(parser.parse());
            }

            const size_t num_workers = runtime.worker_count();

            // Remove old GC roots before re init
            const size_t old_count = script->on_update_functions[scriptIndex].size();
            for (size_t w = 0; w < old_count; ++w) {
                if (auto *func = script->on_update_functions[scriptIndex][w])
                    runtime.get_worker(w)->GetInterpreter().gc.remove_root(func);
                if (auto *func = script->on_destroy_functions[scriptIndex][w])
                    runtime.get_worker(w)->GetInterpreter().gc.remove_root(func);
                if (auto *func = script->on_exit_functions[scriptIndex][w])
                    runtime.get_worker(w)->GetInterpreter().gc.remove_root(func);
            }

            script->instance_envs[scriptIndex].resize(num_workers);
            script->on_update_functions[scriptIndex].assign(num_workers, nullptr);
            script->on_destroy_functions[scriptIndex].assign(num_workers, nullptr);
            script->on_exit_functions[scriptIndex].assign(num_workers, nullptr);

            // per worker env with entity wrappers
            for (size_t w = 0; w < num_workers; ++w) {
                auto *worker = runtime.get_worker(w);
                auto &interp = worker->GetInterpreter();

                script->instance_envs[scriptIndex][w] = worker->copy_globals();
                interp.register_environment(script->instance_envs[scriptIndex][w]);

                auto *entityWrapper = Scripting::CreateEntityObject(&interp, registry, entityId);
                script->instance_envs[scriptIndex][w]->define("this", entityWrapper);
            }

            // run top level code once
            runtime.get_worker(0)->execute(script->ast_nodes[scriptIndex], script->instance_envs[scriptIndex][0]);

            // Bind hook functions
            auto bind_hook = [&](const char *name, std::vector<ObSL::ObSLCallable *> &target) {
                try {
                    const auto val = script->instance_envs[scriptIndex][0]->get(name);
                    if (!std::holds_alternative<ObSL::ObSLCallable *>(val))
                        return;
                    auto *base_func = std::get<ObSL::ObSLCallable *>(val);

                    if (auto *obsl_func = dynamic_cast<ObSL::ObSLFunction *>(base_func)) {
                        for (size_t w = 0; w < num_workers; ++w) {
                            auto *worker_w = runtime.get_worker(w);
                            auto &interp_w = worker_w->GetInterpreter();
                            auto this_val = script->instance_envs[scriptIndex][w]->get("this");
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

            bind_hook("on_update", script->on_update_functions[scriptIndex]);
            bind_hook("on_destroy", script->on_destroy_functions[scriptIndex]);
            bind_hook("on_exit", script->on_exit_functions[scriptIndex]);

            script->isInitialized[scriptIndex] = true;
            if (auto *logger = Logging::LoggerService::Get()) {
                logger->log("ScriptSystem", "Initialized '" + script->scriptPaths[scriptIndex] + "' across " + std::to_string(num_workers) + " worker(s)", Logging::LogSeverity::Info);
            }
        } catch (const std::exception &e) {
            if (auto *logger = Logging::LoggerService::Get()) {
                logger->log("ScriptSystem", "Error compiling/executing '" + script->scriptPaths[scriptIndex] + "':\n  " + e.what(), Logging::LogSeverity::Error);
            }
        }
    }

    inline void Update(Registry &registry, const Core::EngineContext &ctx) {
        if (!ctx.scriptPool || !ctx.threadPool)
            return;
        constexpr ObSL::Token call_token{ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0};
        constexpr uint64_t kReloadPollIntervalFrames = 300;
        const bool shouldPollReload = !IO::VFS::IsPackaged() && ctx.frameCount % kReloadPollIntervalFrames == 0;

        Scripting::ScriptCommandBuffer cmd_buf;
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

        registry.ForEach<Components::ScriptComponent>([&](const Entity entity, Components::ScriptComponent *script) {
            const auto raw_id = static_cast<EntityID>(entity);
            if (script->resolvedScriptPaths.size() != script->scriptPaths.size()) {
                script->resolvedScriptPaths.resize(script->scriptPaths.size());
            }

            for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                if (!script->isInitialized[i]) {
                    pendingInits.push_back({raw_id, script, i, false});
                }

                try {
                    if (shouldPollReload) {
                        if (script->resolvedScriptPaths[i].empty()) {
                            script->resolvedScriptPaths[i] = IO::VFS::Resolve(script->scriptPaths[i]);
                        }
                        if (const std::filesystem::path &resolvedPath = script->resolvedScriptPaths[i]; std::filesystem::exists(resolvedPath)) {
                            if (const auto current_time = std::filesystem::last_write_time(resolvedPath); current_time > script->lastModified[i] && script->isInitialized[i]) {
                                // only treat as a hot reload for already-initialized scripts;
                                // uninitialized scripts are freshly loaded below, so no reload needed
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

        for (const auto &entry : pendingInits) {
            if (!registry.IsValid(entry.entityId))
                continue;
            auto *script = registry.GetComponent<Components::ScriptComponent>(entry.entityId);
            if (!script)
                continue;
            if (entry.scriptIndex >= script->scriptPaths.size())
                continue;
            if (!script->isInitialized[entry.scriptIndex]) {
                InitializeScript(registry, entry.entityId, script, *ctx.scriptPool, entry.scriptIndex);
            } else if (entry.isReload) {
                script->isInitialized[entry.scriptIndex] = false;
                InitializeScript(registry, entry.entityId, script, *ctx.scriptPool, entry.scriptIndex);
                if (auto *logger = Logging::LoggerService::Get()) {
                    logger->log("ScriptSystem", "Hot-reloaded script: " + script->scriptPaths[entry.scriptIndex], Logging::LogSeverity::Info);
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

        registry.ForEach<Components::ScriptComponent>([&](const Entity entity, Components::ScriptComponent *script) {
            const auto entity_id = static_cast<EntityID>(entity);
            for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                if (script->isInitialized[i] && script->on_update_functions[i][0]) {
                    const size_t w = (entity_id + i) % num_workers;
                    s_Buckets[w].push_back({script->on_update_functions[i][w], script->instance_envs[i][w], script->scriptPaths[i], "on_update"});
                    ++totalWork;
                }
            }
        });

        registry.ForEach<Components::DestroyTagComponent>([&](const Entity entity, Components::DestroyTagComponent *) {
            const auto entity_id = static_cast<EntityID>(entity);
            if (const auto script = registry.GetComponent<Components::ScriptComponent>(entity_id)) {
                for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                    if (script->isInitialized[i] && script->on_destroy_functions[i][0]) {
                        const size_t w = (entity_id + i) % num_workers;
                        s_Buckets[w].push_back({script->on_destroy_functions[i][w], script->instance_envs[i][w], script->scriptPaths[i], "on_destroy"});
                        ++totalWork;
                    }
                }
            }
        });

        const std::vector<ObSL::Value> args = {static_cast<double>(ctx.deltaTime)};
        for (size_t w = 0; w < num_workers; ++w) {
            if (s_Buckets[w].empty())
                continue;
            ctx.threadPool->enqueue(static_cast<Platform::Threading::Task>([&ctx, w, &args, &call_token] {
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
            }));
        }

        if (totalWork > 0)
            ctx.threadPool->wait();

        // flush deferred registry writes
        cmd_buf.flush(registry);

        for (size_t w = 0; w < num_workers; ++w)
            ctx.scriptPool->get_worker(w)->clear_frame_context();
    }

    inline void OnSceneExit(Registry &registry, const Core::EngineContext &ctx) {
        if (!ctx.scriptPool || !ctx.threadPool)
            return;
        constexpr ObSL::Token call_token{ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0};

        Scripting::ScriptCommandBuffer cmd_buf;
        const size_t num_workers = ctx.scriptPool->worker_count();

        for (size_t w = 0; w < num_workers; ++w)
            ctx.scriptPool->get_worker(w)->set_frame_context(&cmd_buf);

        size_t active_exits = 0;
        registry.ForEach<Components::ScriptComponent>([&](const Entity, const Components::ScriptComponent *script) {
            for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                if (script->isInitialized[i] && script->on_exit_functions[i][0])
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
            for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                if (script->isInitialized[i] && script->on_exit_functions[i][0]) {
                    const size_t w = (entity_id + i) % exit_workers;
                    buckets[w].push_back({script->on_exit_functions[i][w], script->instance_envs[i][w], script->scriptPaths[i]});
                }
            }
        });

        const double dt = ctx.deltaTime;
        for (size_t w = 0; w < exit_workers; ++w) {
            ctx.threadPool->enqueue(static_cast<Platform::Threading::Task>([&buckets, &ctx, w, dt, &call_token] {
                auto *worker = ctx.scriptPool->get_worker(w);
                auto &interp = worker->GetInterpreter();
                for (auto &[func, env, scriptPath] : buckets[w]) {
                    try {
                        if (func && env) {
                            interp.set_current_environment(env);
                            const std::vector<ObSL::Value> args = {static_cast<double>(dt)};
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
            }));
        }

        ctx.threadPool->wait();
        cmd_buf.flush(registry);

        for (size_t w = 0; w < num_workers; ++w)
            ctx.scriptPool->get_worker(w)->clear_frame_context();
    }
} // namespace ECS::Systems::ScriptSystem
