#pragma once
#include "Interpreter/Interpreter.h"

namespace ObSL {
    // one worker per thread pls :)
    class ScriptWorker {
    public:
        ScriptWorker();

        Interpreter &GetInterpreter() { return m_Interpreter; }

        // clone for per ent sandboxing stuff
        std::shared_ptr<Environment> copy_globals();

        void execute(
            const std::vector<std::unique_ptr<Stmt> > &ast,
            std::shared_ptr<Environment> env
        );

        Value GetVal(const std::string &name, const std::shared_ptr<Environment> &env);

        void SetVal(const std::string &name, const Value &val, const std::shared_ptr<Environment> &env);

        // maybe this name is shit
        GarbageCollector &gc() { return m_Interpreter.gc; }

        void set_frame_context(void *ctx) { m_FrameContext = ctx; }

        void *frame_context() const { return m_FrameContext; }

    private:
        Interpreter m_Interpreter;
        std::shared_ptr<Environment> m_Globals;
        void *m_FrameContext = nullptr;
    };
} // ObSL
