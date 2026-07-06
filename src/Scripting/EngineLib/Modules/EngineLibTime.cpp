#include "../EngineLib.h"
#include <mutex>
#include <ObSL/Interpreter.h>

namespace {
    std::mutex s_TimeMutex;
}

void Scripting::EngineLib::register_time_modules(ObSL::Interpreter &interpreter) {
    interpreter.get_global_environment()->define(
            "GetFrameCount",
            interpreter.gc.allocate<ObSL::NativeFunction>(
                    0,
                    [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                        if (ctx) {
                            std::lock_guard lock(s_TimeMutex);
                            return static_cast<double>(ctx->frameCount);
                        }
                        return 0.0;
                    },
                    "GetFrameCount"));

    interpreter.get_global_environment()->define(
            "GetTimeScale",
            interpreter.gc.allocate<ObSL::NativeFunction>(
                    0,
                    [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                        if (ctx) {
                            std::lock_guard lock(s_TimeMutex);
                            return static_cast<double>(ctx->timeScale);
                        }
                        return 1.0;
                    },
                    "GetTimeScale"));

    interpreter.get_global_environment()->define(
            "SetTimeScale",
            interpreter.gc.allocate<ObSL::NativeFunction>(
                    1,
                    [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                        if (ctx && !args.empty() && std::holds_alternative<double>(args[0])) {
                            std::lock_guard lock(s_TimeMutex);
                            ctx->timeScale = std::max(0.0f, static_cast<float>(std::get<double>(args[0])));
                            return true;
                        }
                        return false;
                    },
                    "SetTimeScale"));

    interpreter.get_global_environment()->define(
            "GetRawDt", interpreter.gc.allocate<ObSL::NativeFunction>(
                                0,
                                [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                                    return ctx ? static_cast<double>(ctx->deltaTime) : 0.0;
                                },
                                "GetRawDt"));
}
