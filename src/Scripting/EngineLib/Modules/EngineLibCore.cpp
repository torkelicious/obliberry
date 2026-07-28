#include "Scripting/EngineLib/EngineLib.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <mutex>
#include "Platform/Window/Window.h"
#include "IO/Loaders/PrefabManager.h"
#include <ObSL/Interpreter.h>

namespace {
    std::mutex g_WindowMutex;
}

void Scripting::EngineLib::register_core_modules(ObSL::Interpreter &interpreter) {
    interpreter.get_global_environment()->define(
            "get_dt", interpreter.gc.allocate<ObSL::NativeFunction>(
                              0, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { return ctx ? static_cast<double>(ctx->deltaTime * ctx->timeScale) : 0.0; }, "get_dt"));

    // window management
    interpreter.get_global_environment()->define("Window_GetHeight", interpreter.gc.allocate<ObSL::NativeFunction>(

                                                                             0,
                                                                             [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                                                                                 if (ctx && ctx->window) {
                                                                                     std::lock_guard lock(g_WindowMutex);
                                                                                     return static_cast<double>(ctx->window->GetHeight());
                                                                                 }
                                                                                 return 0.0;
                                                                             },
                                                                             "Window_GetHeight"));

    interpreter.get_global_environment()->define("Window_GetWidth", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                            0,
                                                                            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                                                                                if (ctx && ctx->window) {
                                                                                    std::lock_guard lock(g_WindowMutex);
                                                                                    return static_cast<double>(ctx->window->GetWidth());
                                                                                }
                                                                                return 0.0;
                                                                            },
                                                                            "Window_GetWidth"));

    interpreter.get_global_environment()->define("Window_SetFullscreen", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                                 1,
                                                                                 [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                                     if (args.empty())
                                                                                         return std::monostate{};
                                                                                     bool fullscreen = false;
                                                                                     if (std::holds_alternative<bool>(args[0])) {
                                                                                         fullscreen = std::get<bool>(args[0]);
                                                                                     } else if (std::holds_alternative<double>(args[0])) {
                                                                                         fullscreen = std::get<double>(args[0]) != 0.0;
                                                                                     } else {
                                                                                         return std::monostate{};
                                                                                     }
                                                                                     if (ctx && ctx->window) {
                                                                                         std::lock_guard lock(g_WindowMutex);
                                                                                         ctx->window->SetFullscreen(fullscreen);
                                                                                     }
                                                                                     return std::monostate{};
                                                                                 },
                                                                                 "Window_SetFullscreen"));

    interpreter.get_global_environment()->define("CloseWindow", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                        0,
                                                                        [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                                                                            if (ctx && ctx->window && ctx->window->GetNativeWindow()) {
                                                                                std::lock_guard lock(g_WindowMutex);
                                                                                glfwSetWindowShouldClose(ctx->window->GetNativeWindow(), true);
                                                                            }
                                                                            return std::monostate{};
                                                                        },
                                                                        "CloseWindow"));
}
