#include "../EngineLib.h"
#include <mutex>
#include <ObSL/Interpreter.h>
#include "Sound/AudioEngine.h"

namespace {
    std::mutex s_AudioMutex;
}

void Scripting::EngineLib::register_audio_modules(ObSL::Interpreter &interpreter) {
    interpreter.get_global_environment()->define("PlaySound2D", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                        2,
                                                                        [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                            if (ctx && ctx->audioEngine && args.size() >= 2) {
                                                                                if (std::holds_alternative<std::string>(args[0]) && std::holds_alternative<double>(args[1])) {
                                                                                    const std::string path = std::get<std::string>(args[0]);
                                                                                    const auto volume = static_cast<float>(std::get<double>(args[1]));
                                                                                    std::lock_guard lock(s_AudioMutex);
                                                                                    ctx->audioEngine->PlaySound2D(path, volume);
                                                                                }
                                                                            }
                                                                            return std::monostate{};
                                                                        },
                                                                        "PlaySound2D"));

    interpreter.get_global_environment()->define("PlayMusic", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                      2,
                                                                      [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                          if (ctx && ctx->audioEngine && args.size() >= 2) {
                                                                              if (std::holds_alternative<std::string>(args[0]) && std::holds_alternative<double>(args[1])) {
                                                                                  const std::string path = std::get<std::string>(args[0]);
                                                                                  const auto volume = static_cast<float>(std::get<double>(args[1]));
                                                                                  std::lock_guard lock(s_AudioMutex);
                                                                                  ctx->audioEngine->PlayMusic(path, volume);
                                                                              }
                                                                          }
                                                                          return std::monostate{};
                                                                      },
                                                                      "PlayMusic"));

    interpreter.get_global_environment()->define("StopMusic", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                      0,
                                                                      [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                                                                          if (ctx && ctx->audioEngine) {
                                                                              std::lock_guard lock(s_AudioMutex);
                                                                              ctx->audioEngine->StopMusic();
                                                                          }
                                                                          return std::monostate{};
                                                                      },
                                                                      "StopMusic"));

    interpreter.get_global_environment()->define("SetMasterVolume", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                            1,
                                                                            [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                                if (ctx && ctx->audioEngine && !args.empty() && std::holds_alternative<double>(args[0])) {
                                                                                    std::lock_guard lock(s_AudioMutex);
                                                                                    ctx->audioEngine->SetMasterVolume(static_cast<float>(std::get<double>(args[0])));
                                                                                }
                                                                                return std::monostate{};
                                                                            },
                                                                            "SetMasterVolume"));
}
