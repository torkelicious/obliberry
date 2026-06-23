#pragma once

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

#include "Core/EngineContext.h"
#include "ECS/Registry.h"
#include "ECS/Components/DestroyTagComponent.h"
#include "ECS/Components/ScriptComponent.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"
#include "Scripting/ObSLCore/Lexer/Lexer.h"
#include "Scripting/ObSLCore/Parser/Parser.h"

ObSL::ObSLObject *CreateEntityObject(ObSL::Interpreter *interpreter, Registry &registry, EntityID id);

namespace ScriptSystem {
    inline void InitializeScript(Registry &registry, EntityID entityId, ScriptComponent *script,
                                 ObSL::Interpreter *interpreter, size_t scriptIndex = 0) {
        if (scriptIndex >= script->scriptPaths.size() || script->isInitialized[scriptIndex] || !interpreter) return;

        // clear any stale function pointers
        script->on_update_functions[scriptIndex] = nullptr;
        script->on_destroy_functions[scriptIndex] = nullptr;
        script->on_exit_functions[scriptIndex] = nullptr;

        // create entity script env
        script->instance_envs[scriptIndex] = std::make_shared<ObSL::Environment>(interpreter->get_global_environment());
        interpreter->register_environment(script->instance_envs[scriptIndex]);

        auto *entityWrapper = CreateEntityObject(interpreter, registry, entityId);
        script->instance_envs[scriptIndex]->define("this", entityWrapper);

        // read file
        std::ifstream file(script->scriptPaths[scriptIndex]);
        if (!file.is_open()) {
            std::cerr << "[ScriptSystem] Error: Failed to open script file: " << script->scriptPaths[scriptIndex] <<
                    "\n";
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        script->source_codes[scriptIndex] = buffer.str();
        script->lastModified[scriptIndex] = std::filesystem::last_write_time(script->scriptPaths[scriptIndex]);

        try {
            ObSL::Lexer lexer(script->source_codes[scriptIndex]);
            std::vector<ObSL::Token> tokens = lexer.tokenize();

            ObSL::Parser parser(tokens);
            script->ast_nodes[scriptIndex] = parser.parse();

            auto prev_env = interpreter->get_current_environment();
            interpreter->set_current_environment(script->instance_envs[scriptIndex]);

            interpreter->interpret(script->ast_nodes[scriptIndex]);

            // look for built in hook functions
            try {
                if (const auto val = script->instance_envs[scriptIndex]->get("on_update"); std::holds_alternative<
                    ObSL::ObSLCallable *>(val)) {
                    script->on_update_functions[scriptIndex] = std::get<ObSL::ObSLCallable *>(val);
                }
            } catch (...) {
            }

            try {
                if (const auto val = script->instance_envs[scriptIndex]->get("on_destroy"); std::holds_alternative<
                    ObSL::ObSLCallable *>(val)) {
                    script->on_destroy_functions[scriptIndex] = std::get<ObSL::ObSLCallable *>(val);
                }
            } catch (...) {
            }

            try {
                if (const auto val = script->instance_envs[scriptIndex]->get("on_exit"); std::holds_alternative<
                    ObSL::ObSLCallable *>(val)) {
                    script->on_exit_functions[scriptIndex] = std::get<ObSL::ObSLCallable *>(val);
                }
            } catch (...) {
            }

            // restore the previous environment
            interpreter->set_current_environment(prev_env);

            script->isInitialized[scriptIndex] = true;
        } catch (const std::exception &e) {
            std::cerr << "[ScriptSystem] Error compiling/executing '" << script->scriptPaths[scriptIndex] << "':\n  " <<
                    e.what() << "\n";
        }
    }

    inline void Update(Registry &registry, const EngineContext &ctx) {
        if (!ctx.scriptEngine) return;
        constexpr ObSL::Token call_token{
            ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0
        };

        registry.ForEach<ScriptComponent>([&](const Entity entity, ScriptComponent *script) {
            const auto raw_id = static_cast<EntityID>(entity);

            for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                if (!script->isInitialized[i]) {
                    InitializeScript(registry, raw_id, script, ctx.scriptEngine, i);
                }

                try {
                    if (std::filesystem::exists(script->scriptPaths[i])) {
                        if (auto current_time = std::filesystem::last_write_time(script->scriptPaths[i]);
                            current_time > script->lastModified[i]) {
                            script->isInitialized[i] = false;
                            InitializeScript(registry, raw_id, script, ctx.scriptEngine, i);
                            std::cout << "[ScriptSystem] Hot-reloaded script: " << script->scriptPaths[i] << "\n";
                        }
                    }
                } catch (const std::exception &e) {
                    std::cerr << "[ScriptSystem] reload error: " << e.what() << "\n";
                }

                try {
                    if (script->isInitialized[i] && script->on_update_functions[i]) {
                        const std::vector<ObSL::Value> args = {static_cast<double>(ctx.deltaTime)};
                        ctx.scriptEngine->set_current_environment(script->instance_envs[i]);
                        script->on_update_functions[i]->call(ctx.scriptEngine, args, call_token);
                    }
                } catch (const std::exception &e) {
                    std::cerr << "[ScriptSystem] Exception in script on_update (" << script->scriptPaths[i] << "): " <<
                            e.what() << "\n";
                } catch (...) {
                    std::cerr << "[ScriptSystem] Unknown Exception in script on_update loop (" << script->scriptPaths[i]
                            << ")\n";
                }
            }
        });

        registry.ForEach<DestroyTagComponent>(
            [&](const Entity entity, DestroyTagComponent *) {
                if (const auto script = registry.GetComponent<ScriptComponent>(static_cast<EntityID>(entity))) {
                    for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                        try {
                            if (script->isInitialized[i] && script->on_destroy_functions[i]) {
                                const std::vector<ObSL::Value> args = {static_cast<double>(ctx.deltaTime)};
                                ctx.scriptEngine->set_current_environment(script->instance_envs[i]);
                                script->on_destroy_functions[i]->call(ctx.scriptEngine, args, call_token);
                            }
                        } catch (const std::exception &e) {
                            std::cerr << "[ScriptSystem] Exception in on_destroy (" << script->scriptPaths[i] << "): "
                                    << e.what() << "\n";
                        } catch (...) {
                            std::cerr << "[ScriptSystem] Unknown Exception in on_destroy loop\n";
                        }
                    }
                }
            });
    }

    inline void OnSceneExit(Registry &registry, const EngineContext &ctx) {
        if (!ctx.scriptEngine) return;
        constexpr ObSL::Token call_token{
            ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0
        };
        registry.ForEach<ScriptComponent>([&](const Entity /*entity*/, const ScriptComponent *script) {
            for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                try {
                    if (script->isInitialized[i] && script->on_exit_functions[i]) {
                        const std::vector<ObSL::Value> args = {static_cast<double>(ctx.deltaTime)};
                        ctx.scriptEngine->set_current_environment(script->instance_envs[i]);
                        script->on_exit_functions[i]->call(ctx.scriptEngine, args, call_token);
                    }
                } catch (const std::exception &e) {
                    std::cerr << "[ScriptSystem] Exception in on_exit (" << script->scriptPaths[i] << "): " << e.what()
                            << "\n";
                } catch (...) {
                    std::cerr << "[ScriptSystem] Unknown Exception in on_exit loop\n";
                }
            }
        });
    }
}
