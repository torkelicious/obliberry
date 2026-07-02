#pragma once
#include <any>
#include "Interpreter/Interpreter.h"

namespace ObSL {
    class ScriptWorker {
    public:
        ScriptWorker();

        Interpreter &GetInterpreter() { return m_Interpreter; }
        const Interpreter &GetInterpreter() const { return m_Interpreter; }

        std::shared_ptr<Environment> copy_globals();

        void execute(
            const std::vector<std::unique_ptr<Stmt>> &ast,
            std::shared_ptr<Environment> env
        );

        Value GetVal(const std::string &name, const std::shared_ptr<Environment> &env);
        void SetVal(const std::string &name, const Value &val, const std::shared_ptr<Environment> &env);

        GarbageCollector &gc() { return m_Interpreter.gc; }
        const GarbageCollector &gc() const { return m_Interpreter.gc; }

        template<typename T>
                void set_frame_context(T *ctx) {
                    m_FrameContext = ctx;
                }

                void clear_frame_context() {
                    m_FrameContext.reset();
                }

                template<typename T>
                T *frame_context() {
                    if (m_FrameContext.has_value() && m_FrameContext.type() == typeid(T *)) {
                        return std::any_cast<T *>(m_FrameContext);
                    }
                    return nullptr;
                }

                template<typename T>
                const T *frame_context() const {
                    if (m_FrameContext.has_value() && m_FrameContext.type() == typeid(T *)) {
                        return std::any_cast<T *>(m_FrameContext);
                    }
                    return nullptr;
                }

    private:
        Interpreter m_Interpreter;
        std::shared_ptr<Environment> m_Globals;
        std::any m_FrameContext;
    };
} // ObSL
