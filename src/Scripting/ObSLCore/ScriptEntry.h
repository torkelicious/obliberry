#pragma once

#include "Interpreter/Interpreter.h"

namespace ObSL {
    class ScriptEntry {
    public:
        ScriptEntry() = default;

        ~ScriptEntry() = default;

        ScriptEntry(const ScriptEntry &) = delete;

        ScriptEntry &operator=(const ScriptEntry &) = delete;

        int exec(int argc, char *argv[]);

    private:
        Interpreter m_interpreter;

        // everything should go through exec()
        void runFile(const std::string &path);

        void runREPL();

        void run(const std::string &source);

        static void run_lint(const std::string &path);
    };
} // ObSL


