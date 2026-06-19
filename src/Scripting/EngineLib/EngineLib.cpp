#include "EngineLib.h"
#include <string>
#include "Renderer/Camera.h"
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"

// very work in progress this is nowhere near finished!!!
void EngineLib::register_modules(ObSL::Interpreter &interpreter) {
    EngineContext *ctx = m_ctx;
    Registry *reg = m_registry;

    interpreter.define_native("print_name", [reg](const double entity_id) -> bool {
        const std::string name = reg->GetEntityName(static_cast<EntityID>(entity_id));
        std::cout << "Entity " << entity_id << " is named: " << name << "\n";
        return true;
    });
}
