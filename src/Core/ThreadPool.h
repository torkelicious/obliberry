#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace Core {
    class ThreadPool {
    public:
        explicit ThreadPool(
            size_t count = std::thread::hardware_concurrency()
        );

        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        void enqueue(std::function<void()> task);
        void wait();
        void stop();

    private:
        std::vector<std::thread> m_Threads;
        std::queue<std::function<void()>> m_Tasks;
        std::mutex m_Mutex;
        std::condition_variable m_CV;
        std::atomic<size_t> m_TasksPending{0};
        std::condition_variable m_DoneCV;
        std::atomic<bool> m_ShouldStop{false};
        std::atomic<bool> m_Stopped{false};
    };
} // Core
