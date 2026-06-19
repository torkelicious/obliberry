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

            ObSL::Token lookup{
                ObSL::TokenType::IDENTIFIER, "on_update", 0, 0, 0, 0
            };

            if (ObSL::Value update_val = script->instance_env->get(lookup); std::holds_alternative<ObSL::ObSLCallable
                *>(update_val)) {
                script->on_update = std::get<ObSL::ObSLCallable *>(update_val);
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
}

#endif //OBLIBERRY_SCRIPTSYSTEM_H
