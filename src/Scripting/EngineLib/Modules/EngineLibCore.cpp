#include "../EngineLib.h"
#include <GLFW/glfw3.h>
#include "Core/Window.h"
#include "IO/PrefabManager.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"

// forward declaration
ObSL::ObSLObject *CreateEntityObject(ObSL::Interpreter *interpreter, ECS::Registry &registry, ECS::EntityID id);

void Scripting::EngineLib::EngineLib::register_core_modules(ObSL::Interpreter &interpreter) {
    interpreter.get_global_environment()->define(
        "get_dt", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return ctx ? static_cast<double>(ctx->deltaTime) : 0.0;
            }, "get_dt"));

    interpreter.get_global_environment()->define(
        "GetEntity", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.empty() || !std::holds_alternative<double>(args[0])) return std::monostate{};

                // the ID that was passed in from the script
                const auto id = static_cast<ECS::EntityID>(std::get<double>(args[0]));

                // return the object
                return CreateEntityObject(interp, *reg, id);
            }, "GetEntity"));

    interpreter.get_global_environment()->define(
        "Find", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
                const auto target_name = std::get<std::string>(args[0]);
                for (const ECS::EntityID id: reg->GetLivingEntities()) {
                    if (reg->GetEntityName(id) == target_name)
                        return CreateEntityObject(interp, *reg, id);
                }
                return std::monostate{};
            }, "Find"));

    interpreter.get_global_environment()->define(
        "CreateEntity", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                std::string name = "NewEntity";
                if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
                    name = std::get<std::string>(args[0]);
                }
                const ECS::EntityID new_id = reg->CreateEntity();
                reg->SetEntityName(new_id, name);
                return CreateEntityObject(interp, *reg, new_id);
            }, "CreateEntity"));

    interpreter.get_global_environment()->define(
        "Instantiate", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [reg = m_registry, ctx = m_ctx](ObSL::Interpreter *interp,
                                            const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.empty() || !std::holds_alternative<std::string>(args[0])) return std::monostate{};
                const std::string prefab_path = std::get<std::string>(args[0]);
                const ECS::EntityID new_id = IO::PrefabManager::Instantiate(*reg, *ctx->resources, prefab_path);
                if (new_id == 0) return std::monostate{};
                return CreateEntityObject(interp, *reg, new_id);
            }, "Instantiate"));

    // window management
    interpreter.get_global_environment()->define(
        "Window_GetHeight", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return ctx && ctx->window ? static_cast<double>(ctx->window->GetHeight()) : 0.0;
            }, "Window_GetHeight"));

    interpreter.get_global_environment()->define(
        "Window_GetWidth", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return ctx && ctx->window ? static_cast<double>(ctx->window->GetWidth()) : 0.0;
            }, "Window_GetWidth"));

    interpreter.get_global_environment()->define(
        "Window_SetFullscreen", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (args.empty()) return std::monostate{};
                bool fullscreen = false;
                // either boolean values or numeric flags
                if (std::holds_alternative<bool>(args[0])) {
                    fullscreen = std::get<bool>(args[0]);
                } else if (std::holds_alternative<double>(args[0])) {
                    fullscreen = std::get<double>(args[0]) != 0.0;
                } else {
                    return std::monostate{};
                }
                if (ctx && ctx->window) {
                    ctx->window->SetFullscreen(fullscreen);
                }

                return std::monostate{};
            }, "Window_SetFullscreen"));

    interpreter.get_global_environment()->define(
        "CloseWindow", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                if (ctx && ctx->window && ctx->window->GetNativeWindow()) {
                    glfwSetWindowShouldClose(ctx->window->GetNativeWindow(), true);
                }
                return std::monostate{};
            }, "CloseWindow"));
}
