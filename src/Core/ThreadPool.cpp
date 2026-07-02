#include "ThreadPool.h"

namespace Core {
    ThreadPool::ThreadPool(const size_t count) {
        m_Threads.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            m_Threads.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(m_Mutex);
                        m_CV.wait(lock, [this] {
                            return !m_Tasks.empty() || m_ShouldStop.load();
                        });

                        if (m_ShouldStop.load() && m_Tasks.empty()) {
                            return;
                        }
                        task = std::move(m_Tasks.front());
                        m_Tasks.pop();
                    }

                    task();

                    {
                        std::unique_lock lock(m_Mutex);
                        --m_TasksPending;
                        if (m_TasksPending == 0) {
                            m_DoneCV.notify_all();
                        }
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

    void ThreadPool::enqueue(std::function<void()> task) {
        {
            std::unique_lock lock(m_Mutex);
            if (m_ShouldStop.load()) return;
            ++m_TasksPending;
            m_Tasks.emplace(std::move(task));
        }
        m_CV.notify_one();
    }

    void ThreadPool::wait() {
        std::unique_lock lock(m_Mutex);
        m_DoneCV.wait(lock, [this] {
            return m_TasksPending == 0;
        });
    }

    void ThreadPool::stop() {
        if (m_Stopped.exchange(true)) return;
        {
            std::unique_lock lock(m_Mutex);
            m_ShouldStop.store(true);
        }
        m_CV.notify_all();
        // Note: threads join in destructor
    }
} // Core
