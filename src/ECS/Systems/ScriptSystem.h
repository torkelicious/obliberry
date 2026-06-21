#ifndef OBLIBERRY_SCRIPTSYSTEM_H
#define OBLIBERRY_SCRIPTSYSTEM_H
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

namespace ScriptSystem {
    inline void InitalizeScript(ScriptComponent *script, ObSL::Interpreter *interpreter, size_t scriptIndex = 0) {
        if (scriptIndex >= script->scriptPaths.size() || script->isInitialized[scriptIndex] || !interpreter) return;

        // create entity script env
        script->instance_envs[scriptIndex] = std::make_shared<ObSL::Environment>(interpreter->get_global_environment());

        // read file
        std::ifstream file(script->scriptPaths[scriptIndex]);
        if (!file.is_open()) {
            std::cerr << "[ScriptSystem] Error: Failed to open script file: " << script->scriptPaths[scriptIndex] <<
                    "\n";
            script->isInitialized[scriptIndex] = true;
            return;
        }
        std::stringstream buff;
        buff << file.rdbuf();

        // store source code permanently in the component
        script->source_codes[scriptIndex] = buff.str();
        if (std::filesystem::exists(script->scriptPaths[scriptIndex])) {
            script->lastModified[scriptIndex] = std::filesystem::last_write_time(script->scriptPaths[scriptIndex]);
        }

        try {
            ObSL::Lexer lexer(script->source_codes[scriptIndex]);
            auto tokens = lexer.tokenize();
            ObSL::Parser parser(tokens);

            script->ast_nodes[scriptIndex] = parser.parse();
            interpreter->interpret(script->ast_nodes[scriptIndex]);

            try {
                if (auto val = script->instance_envs[scriptIndex]->get("on_update"); std::holds_alternative<
                    ObSL::ObSLCallable
                    *>(val)) {
                    script->on_update_functions[scriptIndex] = std::get<ObSL::ObSLCallable *>(val);
                }
            } catch (...) { script->on_update_functions[scriptIndex] = nullptr; }

            try {
                if (auto val = script->instance_envs[scriptIndex]->get("on_destroy"); std::holds_alternative<
                    ObSL::ObSLCallable
                    *>(val)) {
                    script->on_destroy_functions[scriptIndex] = std::get<ObSL::ObSLCallable *>(val);
                }
            } catch (...) { script->on_destroy_functions[scriptIndex] = nullptr; }

            try {
                if (auto val = script->instance_envs[scriptIndex]->get("on_exit"); std::holds_alternative<
                    ObSL::ObSLCallable
                    *>(val)) {
                    script->on_exit_functions[scriptIndex] = std::get<ObSL::ObSLCallable *>(val);
                }
            } catch (...) { script->on_exit_functions[scriptIndex] = nullptr; }

            script->isInitialized[scriptIndex] = true;
        } catch (const std::exception &e) {
            std::cerr << "[ScriptSystem] Script Failed for [" << script->scriptPaths[scriptIndex] << "]:\n  " << e.
                    what() << "\n";
            script->isInitialized[scriptIndex] = true;
        } catch (...) {
            std::cerr << "[ScriptSystem] Unknown error initializing [" << script->scriptPaths[scriptIndex] << "]\n";
            script->isInitialized[scriptIndex] = true;
        }
    }

    inline void Update(Registry &registry, const EngineContext &ctx) {
        if (!ctx.scriptEngine) return;
        constexpr ObSL::Token call_token{
            ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0
        };

        registry.ForEach<ScriptComponent>([&](const Entity entity, ScriptComponent *script) {
            // Process each script in the component
            for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                try {
                    if (!script->isInitialized[i]) {
                        std::cout << "[ScriptSystem] Initializing script: " << script->scriptPaths[i] << " for entity "
                                << static_cast<uint32_t>(entity) << "\n";
                        InitalizeScript(script, ctx.scriptEngine, i);
                    }

                    if (std::filesystem::exists(script->scriptPaths[i])) {
                        if (std::filesystem::last_write_time(script->scriptPaths[i]) != script->lastModified[i]) {
                            std::cout << "Script: " << script->scriptPaths[i] << " was modified, reloading it..\n";
                            script->isInitialized[i] = false;
                        }
                    }

                    if (script->isInitialized[i] && script->on_update_functions[i]) {
                        double raw_id = static_cast<EntityID>(entity);
                        const std::vector<ObSL::Value> args = {raw_id, static_cast<double>(ctx.deltaTime)};
                        ctx.scriptEngine->set_current_environment(script->instance_envs[i]);
                        script->on_update_functions[i]->call(ctx.scriptEngine, args, call_token);
                    }
                } catch (const std::exception &e) {
                    std::cerr << "[ScriptSystem] Runtime Error in Update loop: " << e.what() << "\n";
                    if (script) script->on_update_functions[i] = nullptr; // stop spamming error
                } catch (...) {
                    std::cerr << "[ScriptSystem] Unknown Exception in Update loop!\n";
                    if (script) script->on_update_functions[i] = nullptr;
                }
            }
        });

        registry.ForEach<DestroyTagComponent, ScriptComponent>(
            [&](const Entity entity, DestroyTagComponent *, ScriptComponent *script) {
                // process each script in the component
                for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                    try {
                        if (script && script->isInitialized[i] && script->on_destroy_functions[i]) {
                            double raw_id = static_cast<EntityID>(entity);
                            const std::vector<ObSL::Value> args = {raw_id};

                            ctx.scriptEngine->set_current_environment(script->instance_envs[i]);
                            script->on_destroy_functions[i]->call(ctx.scriptEngine, args, call_token);
                            script->on_destroy_functions[i] = nullptr;
                        }
                    } catch (const std::exception &e) {
                        std::cerr << "[ScriptSystem] Exception in on_destroy: " << e.what() << "\n";
                        if (script) script->on_destroy_functions[i] = nullptr;
                    } catch (...) {
                    }
                }
            });
    }

    inline void OnSceneExit(Registry &registry, const EngineContext &ctx) {
        if (!ctx.scriptEngine) return;
        constexpr ObSL::Token call_token{
            ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0
        };
        registry.ForEach<ScriptComponent>([&](const Entity entity, const ScriptComponent *script) {
            // process each script in the component
            for (size_t i = 0; i < script->scriptPaths.size(); i++) {
                try {
                    if (script->isInitialized[i] && script->on_exit_functions[i]) {
                        double raw_id = static_cast<EntityID>(entity);
                        const std::vector<ObSL::Value> args = {raw_id, static_cast<double>(ctx.deltaTime)};
                        ctx.scriptEngine->set_current_environment(script->instance_envs[i]);
                        script->on_exit_functions[i]->call(ctx.scriptEngine, args, call_token);
                    }
                } catch (const std::exception &e) {
                    std::cerr << "[ScriptSystem] Exception in on_exit: " << e.what() << "\n";
                } catch (...) {
                }
            }
        });
    }
}
#endif // OBLIBERRY_SCRIPTSYSTEM_H
