#pragma once

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

#include "Core/EngineContext.h"
#include "ECS/Registry.h"
#include "ECS/Components/DestroyTagComponent.h"
#include "ECS/Components/ScriptComponent.h"
#include "IO/VFS.h"
#include <ObSL/Lexer.h>
#include <ObSL/ScriptRuntime.h>
#include <ObSL/ScriptWorker.h>
#include <ObSL/Parser.h>
#include "Scripting/EngineLib/ScriptCommandBuffer.h"
#include "Scripting/EngineLib/EngineLibFactories.h"
#include "Core/ThreadPool.h"

namespace ECS::Systems::ScriptSystem {
    inline void InitializeScript(Registry &registry, EntityID entityId,
                                 Components::ScriptComponent *script,
                                 ObSL::ScriptRuntime &runtime, size_t scriptIndex = 0) {
        if (scriptIndex >= script->scriptPaths.size() || script->isInitialized[scriptIndex]) return;

        // read file
        std::filesystem::path resolvedPath = IO::VFS::Resolve(script->scriptPaths[scriptIndex]);
        std::ifstream file(resolvedPath);
        if (!file.is_open()) {
            std::cerr << "[ScriptSystem] Error: Failed to open script file: " << script->scriptPaths[scriptIndex]
                    << " (Resolved: " << resolvedPath.string() << ")\n";
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        script->source_codes[scriptIndex] = buffer.str();
        script->lastModified[scriptIndex] = std::filesystem::last_write_time(resolvedPath);

        try {
            // parse once & share
            ObSL::Lexer lexer(script->source_codes[scriptIndex]);
            std::vector<ObSL::Token> tokens = lexer.tokenize();

            ObSL::Parser parser(tokens);
            script->ast_nodes[scriptIndex] = parser.parse();

            size_t num_workers = runtime.worker_count();

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
            runtime.get_worker(0)->execute(
                script->ast_nodes[scriptIndex],
                script->instance_envs[scriptIndex][0]
            );

            // Bind hook functions
            auto bind_hook = [&](const char *name,
                                 std::vector<ObSL::ObSLCallable *> &target) {
                try {
                    const auto val = script->instance_envs[scriptIndex][0]->get(name);
                    if (!std::holds_alternative<ObSL::ObSLCallable *>(val)) return;
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
            std::cout << "[ScriptSystem] Initialized '" << script->scriptPaths[scriptIndex] << "' across "
                    << num_workers << " worker(s)\n";
        } catch (const std::exception &e) {
            std::cerr << "[ScriptSystem] Error compiling/executing '" << script->scriptPaths[scriptIndex] << "':\n  " <<
                    e.what() << "\n";
        }
    }

    inline void Update(Registry &registry, const Core::EngineContext &ctx) {
        if (!ctx.scriptPool || !ctx.threadPool) return;
        constexpr ObSL::Token call_token{
            ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0
        };

        Scripting::ScriptCommandBuffer cmd_buf;
        const size_t num_workers = ctx.scriptPool->worker_count();

        // Set frame context on all workers so native functions can reach the cmd buf
        for (size_t w = 0; w < num_workers; ++w)
            ctx.scriptPool->get_worker(w)->set_frame_context(&cmd_buf);

        //  Init / hot-reload
        registry.ForEach<Components::ScriptComponent>(
            [&](const Entity entity, Components::ScriptComponent *script) {
                const auto raw_id = static_cast<EntityID>(entity);

                for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                    if (!script->isInitialized[i]) {
                        InitializeScript(registry, raw_id, script, *ctx.scriptPool, i);
                    }

                    try {
                        std::filesystem::path resolvedPath = IO::VFS::Resolve(script->scriptPaths[i]);
                        if (std::filesystem::exists(resolvedPath)) {
                            if (auto current_time = std::filesystem::last_write_time(resolvedPath);
                                current_time > script->lastModified[i]) {
                                script->isInitialized[i] = false;
                                InitializeScript(registry, raw_id, script, *ctx.scriptPool, i);
                                std::cout << "[ScriptSystem] Hot-reloaded script: " << script->scriptPaths[i] << "\n";
                            }
                        }
                    } catch (const std::exception &e) {
                        std::cerr << "[ScriptSystem] reload error: " << e.what() << "\n";
                    }
                }
            });

        // to avoid spinning idle workers
        size_t active_updates = 0;
        registry.ForEach<Components::ScriptComponent>(
            [&](const Entity, Components::ScriptComponent *script) {
                for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                    if (script->isInitialized[i] && script->on_update_functions[i][0])
                        ++active_updates;
                }
            });
        const size_t active_workers = std::min(num_workers, std::max(size_t{1}, active_updates));

        // on_update parallel
        struct UpdateWork {
            ObSL::ObSLCallable *func;
            std::shared_ptr<ObSL::Environment> env;
            std::string scriptPath;
        };
        std::vector<std::vector<UpdateWork> > buckets(active_workers);

        registry.ForEach<Components::ScriptComponent>(
            [&](const Entity entity, Components::ScriptComponent *script) {
                const auto entity_id = static_cast<EntityID>(entity);
                for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                    if (script->isInitialized[i] && script->on_update_functions[i][0]) {
                        const size_t w = (entity_id + i) % active_workers;
                        buckets[w].push_back({
                            script->on_update_functions[i][w],
                            script->instance_envs[i][w],
                            script->scriptPaths[i]
                        });
                    }
                }
            });

        const double dt = ctx.deltaTime;
        for (size_t w = 0; w < active_workers; ++w) {
            ctx.threadPool->enqueue([&buckets, &ctx, w, dt, &call_token] {
                auto *worker = ctx.scriptPool->get_worker(w);
                auto &interp = worker->GetInterpreter();
                for (auto &[func, env, scriptPath]: buckets[w]) {
                    try {
                        if (func && env) {
                            interp.set_current_environment(env);
                            const std::vector<ObSL::Value> args = {static_cast<double>(dt)};
                            func->call(&interp, args, call_token);
                        }
                    } catch (const std::exception &e) {
                        std::cerr << "[ScriptSystem] Exception in on_update (" << scriptPath << "): "
                                << e.what() << "\n";
                    } catch (...) {
                        std::cerr << "[ScriptSystem] Unknown Exception in on_update (" << scriptPath << ")\n";
                    }
                }
            });
        }

        ctx.threadPool->wait();

        // destroyable entities to clamp workers
        size_t active_destroys = 0;
        registry.ForEach<Components::DestroyTagComponent>(
            [&](const Entity entity, Components::DestroyTagComponent *) {
                const auto entity_id = static_cast<EntityID>(entity);
                if (const auto script = registry.GetComponent<Components::ScriptComponent>(entity_id)) {
                    for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                        if (script->isInitialized[i] && script->on_destroy_functions[i][0])
                            ++active_destroys;
                    }
                }
            });
        const size_t destroy_workers = std::min(num_workers, std::max(size_t{1}, active_destroys));

        // on_destroy
        std::vector<std::vector<UpdateWork> > destroy_buckets(destroy_workers);

        registry.ForEach<Components::DestroyTagComponent>(
            [&](const Entity entity, Components::DestroyTagComponent *) {
                const auto entity_id = static_cast<EntityID>(entity);
                if (const auto script = registry.GetComponent<Components::ScriptComponent>(entity_id)) {
                    for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                        if (script->isInitialized[i] && script->on_destroy_functions[i][0]) {
                            const size_t w = (entity_id + i) % destroy_workers;
                            destroy_buckets[w].push_back({
                                script->on_destroy_functions[i][w],
                                script->instance_envs[i][w],
                                script->scriptPaths[i]
                            });
                        }
                    }
                }
            });

        for (size_t w = 0; w < destroy_workers; ++w) {
            ctx.threadPool->enqueue([&destroy_buckets, &ctx, w, dt, &call_token] {
                auto *worker = ctx.scriptPool->get_worker(w);
                auto &interp = worker->GetInterpreter();
                for (auto &[func, env, scriptPath]: destroy_buckets[w]) {
                    try {
                        if (func && env) {
                            interp.set_current_environment(env);
                            const std::vector<ObSL::Value> args = {static_cast<double>(dt)};
                            func->call(&interp, args, call_token);
                        }
                    } catch (const std::exception &e) {
                        std::cerr << "[ScriptSystem] Exception in on_destroy (" << scriptPath << "): "
                                << e.what() << "\n";
                    } catch (...) {
                        std::cerr << "[ScriptSystem] Unknown Exception in on_destroy (" << scriptPath << ")\n";
                    }
                }
            });
        }

        ctx.threadPool->wait();

        // flush deferred registry writes
        cmd_buf.flush(registry);

        for (size_t w = 0; w < num_workers; ++w)
            ctx.scriptPool->get_worker(w)->clear_frame_context();
    }

    inline void OnSceneExit(Registry &registry, const Core::EngineContext &ctx) {
        if (!ctx.scriptPool || !ctx.threadPool) return;
        constexpr ObSL::Token call_token{
            ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0
        };

        Scripting::ScriptCommandBuffer cmd_buf;
        const size_t num_workers = ctx.scriptPool->worker_count();

        for (size_t w = 0; w < num_workers; ++w)
            ctx.scriptPool->get_worker(w)->set_frame_context(&cmd_buf);

        size_t active_exits = 0;
        registry.ForEach<Components::ScriptComponent>(
            [&](const Entity, const Components::ScriptComponent *script) {
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
            std::string scriptPath;
        };
        std::vector<std::vector<ExitWork> > buckets(exit_workers);

        registry.ForEach<Components::ScriptComponent>(
            [&](const Entity entity, const Components::ScriptComponent *script) {
                const auto entity_id = static_cast<EntityID>(entity);
                for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                    if (script->isInitialized[i] && script->on_exit_functions[i][0]) {
                        const size_t w = (entity_id + i) % exit_workers;
                        buckets[w].push_back({
                            script->on_exit_functions[i][w],
                            script->instance_envs[i][w],
                            script->scriptPaths[i]
                        });
                    }
                }
            });

        const double dt = ctx.deltaTime;
        for (size_t w = 0; w < exit_workers; ++w) {
            ctx.threadPool->enqueue([&buckets, &ctx, w, dt, &call_token] {
                auto *worker = ctx.scriptPool->get_worker(w);
                auto &interp = worker->GetInterpreter();
                for (auto &[func, env, scriptPath]: buckets[w]) {
                    try {
                        if (func && env) {
                            interp.set_current_environment(env);
                            const std::vector<ObSL::Value> args = {static_cast<double>(dt)};
                            func->call(&interp, args, call_token);
                        }
                    } catch (const std::exception &e) {
                        std::cerr << "[ScriptSystem] Exception in on_exit (" << scriptPath << "): "
                                << e.what() << "\n";
                    } catch (...) {
                        std::cerr << "[ScriptSystem] Unknown Exception in on_exit (" << scriptPath << ")\n";
                    }
                }
            });
        }

        ctx.threadPool->wait();
        cmd_buf.flush(registry);

        for (size_t w = 0; w < num_workers; ++w)
            ctx.scriptPool->get_worker(w)->clear_frame_context();
    }
} // namespace ECS::Systems::ScriptSystem
