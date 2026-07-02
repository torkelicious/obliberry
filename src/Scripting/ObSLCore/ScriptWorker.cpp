#include "ScriptWorker.h"

namespace ObSL {
    ScriptWorker::ScriptWorker()
        : m_Globals(m_Interpreter.get_global_environment()) {
        m_Interpreter.user_data = this;
    }

    void ScriptWorker::execute(const std::vector<std::unique_ptr<Stmt> > &ast, std::shared_ptr<Environment> env) {
        auto prev = m_Interpreter.get_current_environment();
        m_Interpreter.set_current_environment(std::move(env));
        m_Interpreter.interpret(ast);
        m_Interpreter.set_current_environment(std::move(prev));
    }

    Value ScriptWorker::GetVal(const std::string &name, const std::shared_ptr<Environment> &env) {
        return env->get(name);
    }

    void ScriptWorker::SetVal(const std::string &name, const Value &val, const std::shared_ptr<Environment> &env) {
        env->define(name, val);
    }

    std::shared_ptr<Environment> ScriptWorker::copy_globals() {
        return std::make_shared<Environment>(m_Globals);
    }
} // ObSL
