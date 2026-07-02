#pragma once
#include <memory>
#include <ostream>
#include <vector>

#include "ScriptWorker.h"

namespace ObSL {
    class ScriptRuntime {
    public:
        void init(size_t worker_count);

        ScriptWorker *get_worker(size_t index);

        [[nodiscard]] size_t worker_count() const { return m_Workers.size(); }

        void set_stdout(std::ostream &out);

    private:
        std::vector<std::unique_ptr<ScriptWorker> > m_Workers;
    };
} // ObSL
