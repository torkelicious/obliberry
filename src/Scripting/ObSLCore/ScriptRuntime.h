#pragma once
#include <algorithm>
#include <memory>
#include <thread>
#include <vector>

#include "ScriptWorker.h"

namespace ObSL {
    class ScriptRuntime {
    public:
        void init(size_t worker_count = [] {
            const auto hw = std::thread::hardware_concurrency();
            return std::max<size_t>(1, hw > 2 ? hw - 2u : 1u);
        }());

        [[nodiscard]] ScriptWorker *get_worker(const size_t index) { return m_Workers[index].get(); }
        [[nodiscard]] const ScriptWorker *get_worker(const size_t index) const { return m_Workers[index].get(); }

        [[nodiscard]] size_t worker_count() const { return m_Workers.size(); }

        void set_stdout(std::ostream &out) const;

    private:
        std::vector<std::unique_ptr<ScriptWorker> > m_Workers;
    };
} // ObSL
