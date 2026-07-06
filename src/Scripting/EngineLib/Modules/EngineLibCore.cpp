#include "../EngineLib.h"
#include "../EngineLibFactories.h"
#include <GLFW/glfw3.h>
#include <mutex>
#include <shared_mutex>
#include "Core/Window.h"
#include "IO/PrefabManager.h"
#include <ObSL/Interpreter.h>
#include <ObSL/ScriptWorker.h>

namespace {
    std::mutex g_WindowMutex;
}

void Scripting::EngineLib::register_core_modules(ObSL::Interpreter &interpreter) {
    interpreter.get_global_environment()->define(
            "get_dt", interpreter.gc.allocate<ObSL::NativeFunction>(
                              0,
                              [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                                  return ctx ? static_cast<double>(ctx->deltaTime * ctx->timeScale) : 0.0;
                              },
                              "get_dt"));

    interpreter.get_global_environment()->define(
            "GetEntity",
            interpreter.gc.allocate<ObSL::NativeFunction>(
                    1,
                    [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                        if (args.empty() || !std::holds_alternative<double>(args[0]))
                            return std::monostate{};

                        const auto id = static_cast<ECS::EntityID>(std::get<double>(args[0]));

                        // Read-only
                        std::shared_lock lock(g_RegistryMutex);
                        return CreateEntityObject(interp, *reg, id);
                    },
                    "GetEntity"));

    interpreter.get_global_environment()->define(
            "Find",
            interpreter.gc.allocate<ObSL::NativeFunction>(
                    1,
                    [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                        if (args.empty() || !std::holds_alternative<std::string>(args[0]))
                            return std::monostate{};
                        const auto target_name = std::get<std::string>(args[0]);

                        std::shared_lock lock(g_RegistryMutex);
                        for (const ECS::EntityID id : reg->GetLivingEntities()) {
                            if (reg->GetEntityName(id) == target_name)
                                return CreateEntityObject(interp, *reg, id);
                        }
                        return std::monostate{};
                    },
                    "Find"));

    interpreter.get_global_environment()->define(
            "CreateEntity",
            interpreter.gc.allocate<ObSL::NativeFunction>(
                    1,
                    [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                        std::string name = "NewEntity";
                        if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
                            name = std::get<std::string>(args[0]);
                        }
                        auto *worker =
                                interp->user_data ? static_cast<ObSL::ScriptWorker *>(interp->user_data) : nullptr;
                        auto *cmd_buf = worker ? worker->frame_context<ScriptCommandBuffer>() : nullptr;
                        std::unique_lock lock(g_RegistryMutex);
                        const ECS::EntityID new_id = reg->CreateEntity();
                        reg->SetEntityName(new_id, name);
                        return CreateEntityObject(interp, *reg, new_id);
                    },
                    "CreateEntity"));

    interpreter.get_global_environment()->define(
            "Instantiate",
            interpreter.gc.allocate<ObSL::NativeFunction>(
                    1,
                    [reg = m_registry, ctx = m_ctx](ObSL::Interpreter *interp,
                                                    const std::vector<ObSL::Value> &args) -> ObSL::Value {
                        if (args.empty() || !std::holds_alternative<std::string>(args[0]))
                            return std::monostate{};
                        const std::string prefab_path = std::get<std::string>(args[0]);
                        auto *worker =
                                interp->user_data ? static_cast<ObSL::ScriptWorker *>(interp->user_data) : nullptr;
                        auto *cmd_buf = worker ? worker->frame_context<ScriptCommandBuffer>() : nullptr;
                        std::unique_lock lock(g_RegistryMutex);
                        const ECS::EntityID new_id = IO::PrefabManager::Instantiate(*reg, *ctx->resources, prefab_path);
                        if (new_id == 0)
                            return std::monostate{};
                        return CreateEntityObject(interp, *reg, new_id);
                    },
                    "Instantiate"));

    // window management
    interpreter.get_global_environment()->define(
            "Window_GetHeight",
            interpreter.gc.allocate<ObSL::NativeFunction>(
                    0,
                    [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                        if (ctx && ctx->window) {
                            std::lock_guard lock(g_WindowMutex);
                            return static_cast<double>(ctx->window->GetHeight());
                        }
                        return 0.0;
                    },
                    "Window_GetHeight"));

    interpreter.get_global_environment()->define(
            "Window_GetWidth",
            interpreter.gc.allocate<ObSL::NativeFunction>(
                    0,
                    [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                        if (ctx && ctx->window) {
                            std::lock_guard lock(g_WindowMutex);
                            return static_cast<double>(ctx->window->GetWidth());
                        }
                        return 0.0;
                    },
                    "Window_GetWidth"));

    interpreter.get_global_environment()->define(
            "Window_SetFullscreen",
            interpreter.gc.allocate<ObSL::NativeFunction>(
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

    interpreter.get_global_environment()->define(
            "CloseWindow", interpreter.gc.allocate<ObSL::NativeFunction>(
                                   0,
                                   [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                                       if (ctx && ctx->window && ctx->window->GetNativeWindow()) {
                                           std::lock_guard lock(g_WindowMutex);
                                           glfwSetWindowShouldClose(ctx->window->GetNativeWindow(), true);
                                       }
                                       return std::monostate{};
                                   },
                                   "CloseWindow"));

    interpreter.get_global_environment()->define(
            "DestroyEntity", interpreter.gc.allocate<ObSL::NativeFunction>(
                                     1,
                                     [reg = m_registry](const ObSL::Interpreter *interpreter,
                                                        const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                         if (args.empty() || !std::holds_alternative<double>(args[0]))
                                             return std::monostate{};
                                         const auto id = static_cast<ECS::EntityID>(std::get<double>(args[0]));
                                         auto *worker = static_cast<ObSL::ScriptWorker *>(interpreter->user_data);
                                         auto *cmd_buf = worker->frame_context<ScriptCommandBuffer>();
                                         if (cmd_buf) {
                                             cmd_buf->push([id](ECS::Registry &reg) { reg.DestroyEntity(id); });
                                         } else if (reg) {
                                             std::unique_lock lock(g_RegistryMutex);
                                             reg->DestroyEntity(id);
                                         }
                                         return std::monostate{};
                                     },
                                     "DestroyEntity"));
}
