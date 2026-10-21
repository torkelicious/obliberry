#include "Logger/LoggerService.h"
#include "Scripting/EngineLib/EngineLib.h"
#include "Scripting/EngineLib/ScriptProperties.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <mutex>
#include "Platform/Window/Window.h"
#include <ObSL/Interpreter.h>

namespace {
    std::mutex g_WindowMutex;
}

void Scripting::EngineLib::register_core_modules(ObSL::Interpreter &interpreter) {
    interpreter.get_global_environment()->define("get_dt", interpreter.gc.allocate<ObSL::NativeFunction>(0, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
        return ctx ? static_cast<double>(ctx->deltaTime * ctx->timeScale) : 0.0;
    }, "get_dt"));

    // window management
    interpreter.get_global_environment()->define("Window_GetHeight", interpreter.gc.allocate<ObSL::NativeFunction>(

                                                                             0, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
        if (ctx && ctx->window) {
            std::lock_guard lock(g_WindowMutex);
            return static_cast<double>(ctx->window->GetHeight());
        }
        return 0.0;
    }, "Window_GetHeight"));

    interpreter.get_global_environment()->define("Window_GetWidth", interpreter.gc.allocate<ObSL::NativeFunction>(0, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
        if (ctx && ctx->window) {
            std::lock_guard lock(g_WindowMutex);
            return static_cast<double>(ctx->window->GetWidth());
        }
        return 0.0;
    }, "Window_GetWidth"));

    interpreter.get_global_environment()->define("Window_SetFullscreen", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
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
    }, "Window_SetFullscreen"));

    interpreter.get_global_environment()->define("CloseWindow", interpreter.gc.allocate<ObSL::NativeFunction>(0, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
        if (ctx && ctx->window) {
            std::lock_guard lock(g_WindowMutex);
            if (ctx->isEditorInPlayMode) {
                LOG_INFO("ObSL EngineLib \"CloseWindow\"", "Was called, but will not close the window in EditorMode to save your sanity! :D");
            } else {
                ctx->window->Close();
            }
        }
        return std::monostate{};
    }, "CloseWindow"));


    interpreter.get_global_environment()->define("Prop", interpreter.gc.allocate<ObSL::NativeFunction>(2, [](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        const ObSL::Value &fallback = args[1];
        const PropertyMap *props = CurrentInitProps();
        if (!props || !std::holds_alternative<std::string>(args[0])) {
            return fallback;
        }

        const auto it = props->find(std::get<std::string>(args[0]));
        if (it == props->end()) {
            return fallback;
        }

        const nlohmann::json &stored = it->second;

        if (std::holds_alternative<double>(fallback) && stored.is_number()) {
            return stored.get<double>();
        }
        if (std::holds_alternative<bool>(fallback) && stored.is_boolean()) {
            return stored.get<bool>();
        }
        if (std::holds_alternative<std::string>(fallback) && stored.is_string()) {
            return stored.get<std::string>();
        }
        return fallback;
    }, "Prop"));
}
