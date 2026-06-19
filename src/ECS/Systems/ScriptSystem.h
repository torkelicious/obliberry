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
        if (std::filesystem::exists(script->scriptPath)) {
            script->lastModified = std::filesystem::last_write_time(script->scriptPath);
        }

        try {
            ObSL::Lexer lexer(script->source_code);
            auto tokens = lexer.tokenize();
            ObSL::Parser parser(tokens);

            script->ast_nodes = parser.parse();
            interpreter->interpret(script->ast_nodes);

            try {
                if (auto val = script->instance_env->get("on_update"); std::holds_alternative<ObSL::ObSLCallable
                    *>(val)) {
                    script->on_update = std::get<ObSL::ObSLCallable *>(val);
                }
            } catch (...) { script->on_update = nullptr; }

            try {
                if (auto val = script->instance_env->get("on_destroy"); std::holds_alternative<ObSL::ObSLCallable
                    *>(val)) {
                    script->on_destroy = std::get<ObSL::ObSLCallable *>(val);
                }
            } catch (...) { script->on_destroy = nullptr; }

            try {
                if (auto val = script->instance_env->get("on_exit"); std::holds_alternative<ObSL::ObSLCallable
                    *>(val)) {
                    script->on_exit = std::get<ObSL::ObSLCallable *>(val);
                }
            } catch (...) { script->on_exit = nullptr; }

            script->isInitialized = true;
        } catch (const std::exception &e) {
            std::cerr << "[ScriptSystem] Script Failed for [" << script->scriptPath << "]:\n  " << e.what() << "\n";
            script->isInitialized = true;
        } catch (...) {
            std::cerr << "[ScriptSystem] Unknown error initializing [" << script->scriptPath << "]\n";
            script->isInitialized = true;
        }
    }

    inline void Update(Registry &registry, const EngineContext &ctx) {
        if (!ctx.scriptEngine) return;
        constexpr ObSL::Token call_token{
            ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0
        };

        registry.ForEach<ScriptComponent>([&](const Entity entity, ScriptComponent *script) {
            try {
                if (!script->isInitialized) {
                    InitalizeScript(script, ctx.scriptEngine);
                }

                if (std::filesystem::exists(script->scriptPath)) {
                    if (std::filesystem::last_write_time(script->scriptPath) != script->lastModified) {
                        std::cout << "Script: " << script->scriptPath << " was modified, reloading it..\n";
                        script->isInitialized = false;
                    }
                }

                if (script->isInitialized && script->on_update) {
                    double raw_id = static_cast<EntityID>(entity);
                    const std::vector<ObSL::Value> args = {raw_id, static_cast<double>(ctx.deltaTime)};
                    ctx.scriptEngine->set_current_environment(script->instance_env);
                    script->on_update->call(ctx.scriptEngine, args, call_token);
                }
            } catch (const std::exception &e) {
                std::cerr << "[ScriptSystem] Runtime Error in Update loop: " << e.what() << "\n";
                if (script) script->on_update = nullptr; // Stop spamming error
            } catch (...) {
                std::cerr << "[ScriptSystem] Unknown Exception in Update loop!\n";
                if (script) script->on_update = nullptr;
            }
        });

        registry.ForEach<DestroyTagComponent, ScriptComponent>(
            [&](const Entity entity, DestroyTagComponent *, ScriptComponent *script) {
                try {
                    if (script && script->isInitialized && script->on_destroy) {
                        double raw_id = static_cast<EntityID>(entity);
                        const std::vector<ObSL::Value> args = {raw_id};

                        ctx.scriptEngine->set_current_environment(script->instance_env);
                        script->on_destroy->call(ctx.scriptEngine, args, call_token);
                        script->on_destroy = nullptr;
                    }
                } catch (const std::exception &e) {
                    std::cerr << "[ScriptSystem] Exception in on_destroy: " << e.what() << "\n";
                    if (script) script->on_destroy = nullptr;
                } catch (...) {
                }
            });
    }

    inline void OnSceneExit(Registry &registry, const EngineContext &ctx) {
        if (!ctx.scriptEngine) return;
        constexpr ObSL::Token call_token{
            ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0
        };
        registry.ForEach<ScriptComponent>([&](const Entity entity, const ScriptComponent *script) {
            try {
                if (script->isInitialized && script->on_exit) {
                    double raw_id = static_cast<EntityID>(entity);
                    const std::vector<ObSL::Value> args = {raw_id, static_cast<double>(ctx.deltaTime)};
                    ctx.scriptEngine->set_current_environment(script->instance_env);
                    script->on_exit->call(ctx.scriptEngine, args, call_token);
                }
            } catch (const std::exception &e) {
                std::cerr << "[ScriptSystem] Exception in on_exit: " << e.what() << "\n";
            } catch (...) {
            }
        });
    }
}
#endif // OBLIBERRY_SCRIPTSYSTEM_H
