#include "../EngineLib.h"
#include "Core/InputManager.h"
#include "Renderer/Camera.h"
#include "Core/Window.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"

void EngineLib::register_input_modules(ObSL::Interpreter &interpreter) {
    interpreter.get_global_environment()->define(
        "Input_IsKeyDown", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!ctx || !ctx->input || args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
                return ctx->input->IsKeyDown(InputManager::GetKeyFromName(std::get<std::string>(args[0])));
            }, "Input_IsKeyDown"));

    interpreter.get_global_environment()->define(
        "Input_IsKeyPressed", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!ctx || !ctx->input || args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
                return ctx->input->IsKeyPressed(InputManager::GetKeyFromName(std::get<std::string>(args[0])));
            }, "Input_IsKeyPressed"));

    interpreter.get_global_environment()->define(
        "Input_IsMousePressed", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!ctx || !ctx->input || args.empty() || !std::holds_alternative<double>(args[0])) return false;
                return ctx->input->IsMousePressed(static_cast<int>(std::get<double>(args[0])));
            }, "Input_IsMousePressed"));

    interpreter.get_global_environment()->define(
        "Input_GetMouseWorldPos", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
                if (ctx && ctx->input && ctx->camera && ctx->window) {
                    const auto w = static_cast<float>(ctx->window->GetWidth());
                    const auto h = static_cast<float>(ctx->window->GetHeight());
                    const glm::vec2 m{
                        static_cast<float>(ctx->input->MousePosX()),
                        static_cast<float>(ctx->input->MousePosY())
                    };
                    const glm::vec2 world = ctx->camera->MouseToWorld(m.x, m.y, w, h);
                    obj->fields["x"] = static_cast<double>(world.x);
                    obj->fields["y"] = static_cast<double>(world.y);
                } else {
                    obj->fields["x"] = 0.0;
                    obj->fields["y"] = 0.0;
                }
                return obj;
            }, "Input_GetMouseWorldPos"));
}
