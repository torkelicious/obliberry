#include "StdLib.h"
#include "StdModules.h"
#include "Scripting/Interpreter/Interpreter.h"
#include <stdexcept>

namespace ObSL {
    void StdLib::register_modules(Interpreter &interpreter) {

        interpreter.define_native("assert", [](const bool condition, const std::string &message) -> bool {
            if (!condition) {
                throw std::runtime_error("Assertion Failed: " + message);
            }
            return true;
        });

        // create temporary stack instances & call their register function,
        ConversionLib().register_modules(interpreter);
        MathLib().register_modules(interpreter);
        StringLib().register_modules(interpreter);
        SystemLib().register_modules(interpreter);
        //Reflection().register_modules(interpreter); //not implemented yet
    }
} // ObSL
