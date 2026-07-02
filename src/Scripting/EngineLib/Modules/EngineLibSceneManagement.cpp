#include "../EngineLib.h"
#include <mutex>
#include "Scenes/SceneManager.h"
#include "Scenes/Scene.h"
#include "Core/EngineContext.h"
#include "IO/PrefabManager.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"
#include "Scripting/ObSLCore/ScriptWorker.h"

namespace {
    std::mutex s_SceneMgmtMutex;
}

void Scripting::EngineLib::register_scene_management_modules(ObSL::Interpreter &interpreter) {
    interpreter.get_global_environment()->define(
        "LoadScene", interpreter.gc.allocate<ObSL::NativeFunction>(
            1, // scene file path
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                if (ctx && !args.empty()) {
                    if (std::holds_alternative<std::string>(args[0])) {
                        std::lock_guard lock(s_SceneMgmtMutex);
                        ctx->pendingScenePath = std::get<std::string>(args[0]);
                    }
                }
                return std::monostate{};
            }, "LoadScene"
        ));

    interpreter.get_global_environment()->define(
        "GetCurrentScenePath", interpreter.gc.allocate<ObSL::NativeFunction>(
            0,
            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                if (ctx && ctx->sceneManager) {
                    std::lock_guard lock(s_SceneMgmtMutex);
                    if (const Scenes::Scene *currentScene = ctx->sceneManager->GetCurrentScene()) {
                        return currentScene->GetScenePath();
                    }
                }
                return std::string("");
            }, "GetCurrentScenePath"
        ));
}
