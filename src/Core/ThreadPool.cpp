#include "ThreadPool.h"

namespace Core {
    ThreadPool::ThreadPool(const size_t count) {
        m_Threads.reserve(count);
        for (size_t i = 0; i < count; i++) {
            m_Threads.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(m_Mutex);
                        m_CV.wait(lock, [this] {
                            return !m_Tasks.empty() || m_ShouldStop;
                        });

                        if (m_ShouldStop && m_Tasks
                            .
                            empty()
                        ) {
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
        {
            std::unique_lock lock(m_Mutex);
            m_ShouldStop = true;
        }
        m_CV.notify_all();

        for (auto &thread: m_Threads) {
            thread.join();
        }
    }

    void ThreadPool::pushToQ(std::function<void()> task) {
        {
            std::unique_lock lock(m_Mutex);
            m_TasksPending++;
            m_Tasks.emplace(std::move(task));
        }
        m_CV.notify_one();
    }

    void ThreadPool::wait_all() {
        std::unique_lock lock(m_Mutex);
        m_DoneCV.wait(lock, [this] {
            return m_TasksPending == 0;
        });
    }
} // Core
