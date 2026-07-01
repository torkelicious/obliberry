#include <imgui.h>
#include "Scripting/EngineLib/EngineLib.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"

void Scripting::EngineLib::register_gui_modules(ObSL::Interpreter &interpreter) {
    // temp imgui binding for testing type stuff :)
    interpreter.get_global_environment()->define("ImGui_Button", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                     1,
                                                     [](ObSL::Interpreter *,
                                                        const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                         if (args.empty() || !std::holds_alternative<std::string>(
                                                                 args[0]))
                                                             return false;
                                                         return ImGui::Button(std::get<std::string>(args[0]).c_str());
                                                     }, "ImGui_Button"));
}

