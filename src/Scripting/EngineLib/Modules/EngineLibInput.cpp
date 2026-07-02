#define GLFW_INCLUDE_NONE
#include "../EngineLib.h"
#include <mutex>
#include "Core/InputManager.h"
#include "Rendering/Camera.h"
#include "Core/Window.h"
#include "Rendering/Renderer.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"
#include "Scripting/ObSLCore/ScriptWorker.h"

namespace {
    std::mutex s_InputCameraMutex;
}

void Scripting::EngineLib::register_input_modules(ObSL::Interpreter &interpreter) {
    // KEYBOARD
    interpreter.get_global_environment()->define(
        "Input_IsKeyDown", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!ctx || !ctx->input || args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
                return ctx->input->IsKeyDown(Core::InputManager::GetKeyFromName(std::get<std::string>(args[0])));
            }, "Input_IsKeyDown"));

    interpreter.get_global_environment()->define(
        "Input_IsKeyPressed", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!ctx || !ctx->input || args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
                return ctx->input->IsKeyPressed(Core::InputManager::GetKeyFromName(std::get<std::string>(args[0])));
            }, "Input_IsKeyPressed"));

    interpreter.get_global_environment()->define(
        "Input_IsKeyReleased", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!ctx || !ctx->input || args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
                return ctx->input->IsKeyReleased(Core::InputManager::GetKeyFromName(std::get<std::string>(args[0])));
            }, "Input_IsKeyReleased"));

    // MOUSE BUTTON
    interpreter.get_global_environment()->define(
        "Input_IsMouseDown", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!ctx || !ctx->input || args.empty() || !std::holds_alternative<double>(args[0])) return false;
                return ctx->input->IsMouseDown(static_cast<int>(std::get<double>(args[0])));
            }, "Input_IsMouseDown"));

    interpreter.get_global_environment()->define(
        "Input_IsMousePressed", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!ctx || !ctx->input || args.empty() || !std::holds_alternative<double>(args[0])) return false;
                return ctx->input->IsMousePressed(static_cast<int>(std::get<double>(args[0])));
            }, "Input_IsMousePressed"));

    interpreter.get_global_environment()->define(
        "Input_IsMouseReleased", interpreter.gc.allocate<ObSL::NativeFunction>(
            1,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (!ctx || !ctx->input || args.empty() || !std::holds_alternative<double>(args[0])) return false;
                return ctx->input->IsMouseReleased(static_cast<int>(std::get<double>(args[0])));
            }, "Input_IsMouseReleased"));


    // MOUSE POS / SCROLLING
    interpreter.get_global_environment()->define(
        "Input_GetMouseX", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return ctx && ctx->input ? ctx->input->MousePosX() : 0.0;
            }, "Input_GetMouseX"));

    interpreter.get_global_environment()->define(
        "Input_GetMouseY", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return ctx && ctx->input ? ctx->input->MousePosY() : 0.0;
            }, "Input_GetMouseY"));

    interpreter.get_global_environment()->define(
        "Input_GetScrollX", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return ctx && ctx->input ? ctx->input->ScrollX() : 0.0;
            }, "Input_GetScrollX"));

    interpreter.get_global_environment()->define(
        "Input_GetScrollY", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                return ctx && ctx->input ? ctx->input->ScrollY() : 0.0;
            }, "Input_GetScrollY"));

    interpreter.get_global_environment()->define(
        "Input_GetMouseWorldPos", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
                if (ctx && ctx->input && ctx->camera && ctx->window) {
                    std::lock_guard lock(s_InputCameraMutex);
                    float w = static_cast<float>(ctx->window->GetWidth());
                    float h = static_cast<float>(ctx->window->GetHeight());
                    if (ctx->renderer) {
                        if (const auto fbo = ctx->renderer->GetEditorFramebuffer()) {
                            w = static_cast<float>(fbo->GetWidth());
                            h = static_cast<float>(fbo->GetHeight());
                        }
                    }
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
