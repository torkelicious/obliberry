#ifndef OBLIBERRY_SCRIPTSYSTEM_H
#define OBLIBERRY_SCRIPTSYSTEM_H
#include <fstream>
#include <sstream>
#include <iostream>

#include "Core/EngineContext.h"
#include "ECS/Registry.h"
#include "ECS/Components/ScriptComponent.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"
#include "Scripting/ObSLCore/Lexer/Lexer.h"
#include "Scripting/ObSLCore/Parser/Parser.h"

namespace ScriptSystem {
    inline void InitalizeScript(ScriptComponent *script, ObSL::Interpreter *interpreter) {
        if (script->isInitialized || !interpreter) return;

        // create entity script env
        script->instance_env = std::make_shared<ObSL::Environment>(interpreter->get_global_environment());

        // read file
        std::ifstream file(script->scriptPath);
        if (!file.is_open()) {
            std::cerr << "[ScriptSystem] Error: Failed to open script file: " << script->scriptPath << "\n";
            script->isInitialized = true;
            return;
        }
        std::stringstream buff;
        buff << file.rdbuf();

        // store source code permanently in the component
        script->source_code = buff.str();
        script->lastModified = std::filesystem::last_write_time(script->scriptPath);

        try {
            ObSL::Lexer lexer(script->source_code);
            auto tokens = lexer.tokenize();
            ObSL::Parser parser(tokens);

            // store the AST nodes permanently in the component
            script->ast_nodes = parser.parse();

            interpreter->set_current_environment(script->instance_env);

            // Interpret
            interpreter->interpret(script->ast_nodes);

            ObSL::Token lookup_update{
                ObSL::TokenType::IDENTIFIER, "on_update", 0, 0, 0, 0
            };

            ObSL::Token lookup_destroy{
                ObSL::TokenType::IDENTIFIER, "on_destroy", 0, 0, 0, 0
            };

            ObSL::Token lookup_exit{
                ObSL::TokenType::IDENTIFIER, "on_exit", 0, 0, 0, 0
            };

            if (ObSL::Value update_val = script->instance_env->get(lookup_update); std::holds_alternative<
                ObSL::ObSLCallable
                *>(update_val)) {
                script->on_update = std::get<ObSL::ObSLCallable *>(update_val);
            }

            if (ObSL::Value destroy_val = script->instance_env->get(lookup_destroy); std::holds_alternative<
                ObSL::ObSLCallable
                *>(destroy_val)) {
                script->on_destroy = std::get<ObSL::ObSLCallable *>(destroy_val);
            }

            if (ObSL::Value exit_val = script->instance_env->get(lookup_exit); std::holds_alternative<
                ObSL::ObSLCallable
                *>(exit_val)) {
                script->on_exit = std::get<ObSL::ObSLCallable *>(exit_val);
            }
        } catch (const std::exception &e) {
            std::cerr << "[ScriptSystem] Script Failed for [" << script->scriptPath << "]:\n  " << e.what() << "\n";
            script->on_update = nullptr;
        } catch (...) {
            std::cerr << "[ScriptSystem] Unknown error occurred for [" << script->scriptPath << "]\n";
            script->on_update = nullptr;
        }

        script->isInitialized = true;
    }


    inline void Update(Registry &registry, const EngineContext &ctx) noexcept {
        if (!ctx.scriptEngine) return;
        constexpr ObSL::Token call_token{
            ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0
        };

        registry.ForEach<ScriptComponent>([&](const Entity entity, ScriptComponent *script) {
            if (!script->isInitialized) {
                InitalizeScript(script, ctx.scriptEngine);
            }

            // hot reloading
            if (std::filesystem::last_write_time(script->scriptPath) != script->lastModified) {
                std::cout << "Script: " << script->scriptPath << " was modified, reloading it..\n";
                script->isInitialized = false;
            }

            if (script->on_update) {
                double raw_id = static_cast<EntityID>(entity);
                const std::vector<ObSL::Value> args = {raw_id, static_cast<double>(ctx.deltaTime)};
                ctx.scriptEngine->set_current_environment(script->instance_env);
                script->on_update->call(ctx.scriptEngine, args, call_token);
            }
        });
    }

    // todo: actually call when entity destroyed
    inline void OnEntityDestroyed(const EntityID entity, Registry &registry, const EngineContext &ctx) noexcept {
        if (!ctx.scriptEngine) return;

        if (registry.HasComponent<ScriptComponent>(entity)) {
            if (const ScriptComponent *script = registry.GetComponent<ScriptComponent>(entity);
                script && script->isInitialized && script->on_destroy) {
                constexpr ObSL::Token call_token{
                    ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0
                };

                auto raw_id = static_cast<double>(entity);
                const std::vector<ObSL::Value> args = {raw_id};

                ctx.scriptEngine->set_current_environment(script->instance_env);
                script->on_destroy->call(ctx.scriptEngine, args, call_token);
            }
        }
    }

    inline void OnSceneExit(Registry &registry, const EngineContext &ctx) noexcept {
        if (!ctx.scriptEngine) return;
        constexpr ObSL::Token call_token{
            ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0
        };
        registry.ForEach<ScriptComponent>([&](const Entity entity, const ScriptComponent *script) {
            if (script->isInitialized && script->on_exit) {
                double raw_id = static_cast<EntityID>(entity);
                const std::vector<ObSL::Value> args = {raw_id, static_cast<double>(ctx.deltaTime)};
                ctx.scriptEngine->set_current_environment(script->instance_env);
                script->on_exit->call(ctx.scriptEngine, args, call_token);
            }
        });
    }
}

#endif //OBLIBERRY_SCRIPTSYSTEM_H
