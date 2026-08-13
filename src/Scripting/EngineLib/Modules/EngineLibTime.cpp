#include "Logger/LoggerService.h"
#include "Platform/Timeout.h"
#include "Scripting/EngineLib/EngineLib.h"
#include "Scripting/EngineLib/ScriptCommandBuffer.h"
#include <mutex>
#include <ObSL/Interpreter.h>
#include <ObSL/ScriptWorker.h>

namespace {
    std::mutex s_TimeMutex;
}

void Scripting::EngineLib::register_time_modules(ObSL::Interpreter &interpreter) {
    interpreter.get_global_environment()->define("GetFrameCount", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                          0,
                                                                          [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                                                                              if (ctx) {
                                                                                  std::lock_guard lock(s_TimeMutex);
                                                                                  return static_cast<double>(ctx->frameCount);
                                                                              }
                                                                              return 0.0;
                                                                          },
                                                                          "GetFrameCount"));

    interpreter.get_global_environment()->define("GetTimeScale", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                         0,
                                                                         [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                                                                             if (ctx) {
                                                                                 std::lock_guard lock(s_TimeMutex);
                                                                                 return static_cast<double>(ctx->timeScale);
                                                                             }
                                                                             return 1.0;
                                                                         },
                                                                         "GetTimeScale"));

    interpreter.get_global_environment()->define("SetTimeScale", interpreter.gc.allocate<ObSL::NativeFunction>(
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
            "GetRawDt",
            interpreter.gc.allocate<ObSL::NativeFunction>(0, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { return ctx ? static_cast<double>(ctx->deltaTime) : 0.0; }, "GetRawDt"));


    //this a lil janky
    interpreter.get_global_environment()->define("SetTimeout", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                       2,
                                                                       // func, ms
                                                                       [reg = m_registry](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                           if (args.size() < 2 || !std::holds_alternative<ObSL::ObSLCallable *>(args[0]) || !std::holds_alternative<double>(args[1]))
                                                                               return std::monostate{};

                                                                           auto *fn = std::get<ObSL::ObSLCallable *>(args[0]);
                                                                           const auto ms = std::chrono::milliseconds(static_cast<long long>(std::get<double>(args[1])));

                                                                           // keep fn alive until timer fires
                                                                           interp->gc.add_root(fn);

                                                                           auto call = [interp, fn, reg]() {
                                                                               auto *worker = static_cast<ObSL::ScriptWorker *>(interp->user_data);

                                                                               // give the deferred code a command buffer
                                                                               ScriptCommandBuffer buf;
                                                                               worker->set_frame_context(&buf);
                                                                               try {
                                                                                   constexpr ObSL::Token call_token{ObSL::TokenType::LEFT_PAREN, "(", 0, 0, 0, 0};
                                                                                   fn->call(interp, {}, call_token);
                                                                               } catch (const std::exception &e) {
                                                                                   if (auto *logger = Logging::LoggerService::Get())
                                                                                       logger->log("EngineLib", "SetTimeout callback error: " + std::string(e.what()), Logging::LogSeverity::Error);
                                                                               }
                                                                               worker->clear_frame_context();

                                                                               if (reg)
                                                                                   buf.flush(*reg);

                                                                               // release after the callback
                                                                               interp->gc.remove_root(fn);
                                                                           };

                                                                           Platform::Time::setTimeout(ms, Platform::Threading::SmallTask(std::move(call)));
                                                                           return true;
                                                                       },
                                                                       "SetTimeout"));
}
