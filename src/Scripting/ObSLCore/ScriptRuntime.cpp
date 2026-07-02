#include "ScriptRuntime.h"

namespace ObSL {
    void ScriptRuntime::init(const size_t worker_count) {
        if (!m_Workers.empty()) {
            return; // Already initialized
        }
        m_Workers.reserve(worker_count);
        for (size_t i = 0; i < worker_count; ++i) {
            m_Workers.push_back(std::make_unique<ScriptWorker>());
        }
    }

    void ScriptRuntime::set_stdout(std::ostream &out) {
        for (auto &worker : m_Workers) {
            worker->GetInterpreter().Set_Stdout(out);
        }
    }
} // ObSL
