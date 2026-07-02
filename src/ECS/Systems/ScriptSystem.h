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
#include "Scripting/ObSLCore/Lexer/Lexer.h"
#include "Scripting/ObSLCore/ScriptRuntime.h"
#include "Scripting/ObSLCore/ScriptWorker.h"
#include "Scripting/ObSLCore/Parser/Parser.h"
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

            // Initialize each and cache function pointers
            size_t num_workers = runtime.worker_count();
            script->instance_envs[scriptIndex].resize(num_workers);
            script->on_update_functions[scriptIndex].resize(num_workers, nullptr);
            script->on_destroy_functions[scriptIndex].resize(num_workers, nullptr);
            script->on_exit_functions[scriptIndex].resize(num_workers, nullptr);

            for (size_t w = 0; w < num_workers; ++w) {
                auto *worker = runtime.get_worker(w);
                auto &interp = worker->GetInterpreter();

                // create entity script env as a child of globals
                script->instance_envs[scriptIndex][w] = worker->copy_globals();
                interp.register_environment(script->instance_envs[scriptIndex][w]);

                auto *entityWrapper = Scripting::CreateEntityObject(&interp, registry, entityId);
                script->instance_envs[scriptIndex][w]->define("this", entityWrapper);

                // execute the script in the entity's environment
                worker->execute(script->ast_nodes[scriptIndex], script->instance_envs[scriptIndex][w]);

                // look for built in hook functions
                try {
                    if (const auto val = script->instance_envs[scriptIndex][w]->get("on_update"); std::holds_alternative
                        <
                            ObSL::ObSLCallable *>(val)) {
                        script->on_update_functions[scriptIndex][w] = std::get<ObSL::ObSLCallable *>(val);
                    }
                } catch (...) {
                }

                try {
                    if (const auto val = script->instance_envs[scriptIndex][w]->get("on_destroy");
                        std::holds_alternative<
                            ObSL::ObSLCallable *>(val)) {
                        script->on_destroy_functions[scriptIndex][w] = std::get<ObSL::ObSLCallable *>(val);
                    }
                } catch (...) {
                }

                try {
                    if (const auto val = script->instance_envs[scriptIndex][w]->get("on_exit"); std::holds_alternative<
                        ObSL::ObSLCallable *>(val)) {
                        script->on_exit_functions[scriptIndex][w] = std::get<ObSL::ObSLCallable *>(val);
                    }
                } catch (...) {
                }
            }

            script->isInitialized[scriptIndex] = true;
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

        // on_update parallel
        struct UpdateWork {
            ObSL::ObSLCallable *func;
            std::shared_ptr<ObSL::Environment> env;
            std::string scriptPath;
        };
        std::vector<std::vector<UpdateWork> > buckets(num_workers);

        registry.ForEach<Components::ScriptComponent>(
            [&](const Entity entity, Components::ScriptComponent *script) {
                const auto entity_id = static_cast<EntityID>(entity);
                for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                    if (script->isInitialized[i] && script->on_update_functions[i][0]) {
                        const size_t w = entity_id % num_workers;
                        buckets[w].push_back({
                            script->on_update_functions[i][w],
                            script->instance_envs[i][w],
                            script->scriptPaths[i]
                        });
                    }
                }
            });

        const double dt = ctx.deltaTime;
        for (size_t w = 0; w < num_workers; ++w) {
            ctx.threadPool->pushToQ([&buckets, &ctx, w, dt, &call_token]() {
                auto *worker = ctx.scriptPool->get_worker(w);
                auto &interp = worker->GetInterpreter();
                for (auto &work: buckets[w]) {
                    try {
                        if (work.func && work.env) {
                            interp.set_current_environment(work.env);
                            const std::vector<ObSL::Value> args = {static_cast<double>(dt)};
                            work.func->call(&interp, args, call_token);
                        }
                    } catch (const std::exception &e) {
                        std::cerr << "[ScriptSystem] Exception in on_update (" << work.scriptPath << "): "
                                << e.what() << "\n";
                    } catch (...) {
                        std::cerr << "[ScriptSystem] Unknown Exception in on_update (" << work.scriptPath << ")\n";
                    }
                }
            });
        }

        ctx.threadPool->wait_all();

        // on_destroy
        std::vector<std::vector<UpdateWork> > destroy_buckets(num_workers);

        registry.ForEach<Components::DestroyTagComponent>(
            [&](const Entity entity, Components::DestroyTagComponent *) {
                const auto entity_id = static_cast<EntityID>(entity);
                if (const auto script = registry.GetComponent<Components::ScriptComponent>(entity_id)) {
                    for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                        if (script->isInitialized[i] && script->on_destroy_functions[i][0]) {
                            const size_t w = entity_id % num_workers;
                            destroy_buckets[w].push_back({
                                script->on_destroy_functions[i][w],
                                script->instance_envs[i][w],
                                script->scriptPaths[i]
                            });
                        }
                    }
                }
            });

        for (size_t w = 0; w < num_workers; ++w) {
            ctx.threadPool->pushToQ([&destroy_buckets, &ctx, w, dt, &call_token]() {
                auto *worker = ctx.scriptPool->get_worker(w);
                auto &interp = worker->GetInterpreter();
                for (auto &work: destroy_buckets[w]) {
                    try {
                        if (work.func && work.env) {
                            interp.set_current_environment(work.env);
                            const std::vector<ObSL::Value> args = {static_cast<double>(dt)};
                            work.func->call(&interp, args, call_token);
                        }
                    } catch (const std::exception &e) {
                        std::cerr << "[ScriptSystem] Exception in on_destroy (" << work.scriptPath << "): "
                                << e.what() << "\n";
                    } catch (...) {
                        std::cerr << "[ScriptSystem] Unknown Exception in on_destroy (" << work.scriptPath << ")\n";
                    }
                }
            });
        }

        ctx.threadPool->wait_all();

        // flush deferred registry writes
        cmd_buf.flush(registry);

        for (size_t w = 0; w < num_workers; ++w)
            ctx.scriptPool->get_worker(w)->set_frame_context(nullptr);
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

        // collect on_exit  stuff
        struct ExitWork {
            ObSL::ObSLCallable *func;
            std::shared_ptr<ObSL::Environment> env;
            std::string scriptPath;
        };
        std::vector<std::vector<ExitWork> > buckets(num_workers);

        registry.ForEach<Components::ScriptComponent>(
            [&](const Entity entity, const Components::ScriptComponent *script) {
                const auto entity_id = static_cast<EntityID>(entity);
                for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                    if (script->isInitialized[i] && script->on_exit_functions[i][0]) {
                        const size_t w = entity_id % num_workers;
                        buckets[w].push_back({
                            script->on_exit_functions[i][w],
                            script->instance_envs[i][w],
                            script->scriptPaths[i]
                        });
                    }
                }
            });

        const double dt = ctx.deltaTime;
        for (size_t w = 0; w < num_workers; ++w) {
            ctx.threadPool->pushToQ([&buckets, &ctx, w, dt, &call_token]() {
                auto *worker = ctx.scriptPool->get_worker(w);
                auto &interp = worker->GetInterpreter();
                for (auto &work: buckets[w]) {
                    try {
                        if (work.func && work.env) {
                            interp.set_current_environment(work.env);
                            const std::vector<ObSL::Value> args = {static_cast<double>(dt)};
                            work.func->call(&interp, args, call_token);
                        }
                    } catch (const std::exception &e) {
                        std::cerr << "[ScriptSystem] Exception in on_exit (" << work.scriptPath << "): "
                                << e.what() << "\n";
                    } catch (...) {
                        std::cerr << "[ScriptSystem] Unknown Exception in on_exit (" << work.scriptPath << ")\n";
                    }
                }
            });
        }

        ctx.threadPool->wait_all();
        cmd_buf.flush(registry);

        for (size_t w = 0; w < num_workers; ++w)
            ctx.scriptPool->get_worker(w)->set_frame_context(nullptr);
    }
} // namespace ECS::Systems::ScriptSystem
