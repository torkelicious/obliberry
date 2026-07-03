#pragma once

#include <unordered_map>
#include <string>
#include <ObSL/Interpreter.h>


namespace ECS::Components {
    // this is a generic container for custom Components defined in ObSL scripts.
    struct CustomDataComponent {
        std::unordered_map<std::string, ObSL::Value> script_components;
    };
} // namespace ECS::Components

