#include <imgui.h>
#include <mutex>
#include "Scripting/EngineLib/EngineLib.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"
#include "Scripting/ObSLCore/ScriptWorker.h"

namespace {
    std::mutex s_GuiMutex;
}

void Scripting::EngineLib::register_gui_modules(ObSL::Interpreter &interpreter) {
    // temp imgui binding for testing type stuff :)
    interpreter.get_global_environment()->define("ImGui_Button", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                     1,
                                                     [](ObSL::Interpreter *,
                                                        const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                         if (args.empty() || !std::holds_alternative<std::string>(
                                                                 args[0]))
                                                             return false;
                                                         std::lock_guard lock(s_GuiMutex);
                                                         return ImGui::Button(std::get<std::string>(args[0]).c_str());
                                                     }, "ImGui_Button"));
}

