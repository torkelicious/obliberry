#ifndef OBLIBERRY_ENTRY_H
#define OBLIBERRY_ENTRY_H
#include "Interpreter/Interpreter.h"

namespace ObSL {
    class Entry {
    public:
        Entry() = default;

        ~Entry() = default;

        Entry(const Entry &) = delete;

        Entry &operator=(const Entry &) = delete;

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

#endif //OBLIBERRY_ENTRY_H
