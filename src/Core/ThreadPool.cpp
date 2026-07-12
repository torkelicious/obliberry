#include "ThreadPool.h"
#include "Core/LoggerService.h"

namespace Core {

    constexpr auto LOG_WHO = "ThreadPool";
    ThreadPool::ThreadPool(const size_t count) {
        LOG_INFO(LOG_WHO, "Initialized with " + std::to_string(count) + " thread(s)");
        m_Threads.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            m_Threads.emplace_back([this] {
                while (true) {
                    Task task;
                    {
                        std::unique_lock lock(m_Mutex);
                        m_CV.wait(lock, [this] { return !m_Tasks.empty() || m_ShouldStop.load(); });

                        if (m_ShouldStop.load() && m_Tasks.empty()) {
                            return;
                        }
                        task = std::move(m_Tasks.front());
                        m_Tasks.pop();
                    }

                    if (task)
                        task();

                    // lock-free decrement
                    if (m_TasksPending.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                        // was 1, now 0 then all tasks complete.
                        std::unique_lock lock(m_Mutex);
                        m_DoneCV.notify_all();
                    }
                }
            });
        }
    }

    ThreadPool::~ThreadPool() {
        stop();
        for (auto &thread : m_Threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void ThreadPool::enqueue(Task task) {
        {
            std::unique_lock lock(m_Mutex);
            if (m_ShouldStop.load())
                return;
            ++m_TasksPending;
            m_Tasks.emplace(std::move(task));
        }
        m_CV.notify_one();
    }

    void ThreadPool::wait() {
        std::unique_lock lock(m_Mutex);
        m_DoneCV.wait(lock, [this] { return m_TasksPending.load(std::memory_order_acquire) == 0; });
    }

    void ThreadPool::stop() {
        if (m_Stopped.exchange(true))
            return;
        {
            std::unique_lock lock(m_Mutex);
            m_ShouldStop.store(true);
        }
        m_CV.notify_all();
        // Note: threads join in destructor
    }
} // namespace Core
