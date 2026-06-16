#pragma once

namespace ObSL {
    class Interpreter;

    class Lib {
    public:
        virtual ~Lib() = default;

        virtual void register_modules(Interpreter &interpreter) = 0;
    };

    class StdLib {
    public:
        static void register_modules(Interpreter &interpreter);
    };
} // ObSL
