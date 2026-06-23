#pragma once

#include <unordered_map>
#include <string>
#include "Scripting/ObSLCore/Interpreter/Interpreter.h"


// this is a generic container for custom Components defined in ObSL scripts.
struct CustomDataComponent {
    std::unordered_map<std::string, ObSL::Value> script_components;
};

