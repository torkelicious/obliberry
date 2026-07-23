#include <mutex>
#include "Scripting/EngineLib/EngineLib.h"
#include <ObSL/Interpreter.h>

namespace {
    std::mutex s_GuiMutex;
}

void Scripting::EngineLib::register_gui_modules(ObSL::Interpreter &interpreter) {

}

