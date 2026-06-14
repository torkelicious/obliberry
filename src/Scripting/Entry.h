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

        void runFile(const std::string &path);

        void runREPL();

    private:
        Interpreter m_interpreter;

        void run(const std::string &source);
    };
} // ObSL

#endif //OBLIBERRY_ENTRY_H
