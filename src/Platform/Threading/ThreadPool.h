#pragma once
#include "Task.h"
#include "Core/Constants.h"


#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>


namespace Platform::Threading {

    class TaskGroup {
    public:
        void Add() { m_Pending.fetch_add(1, std::memory_order_relaxed); }
        void Done() {
            if (m_Pending.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                std::unique_lock lock(m_Mutex);
                m_CV.notify_all();
            }
        }
        void Wait() {
            std::unique_lock lock(m_Mutex);
            m_CV.wait(lock, [this] { return m_Pending.load(std::memory_order_acquire) == 0; });
        }

    private:
        std::atomic<int> m_Pending{0};
        std::mutex m_Mutex;
        std::condition_variable m_CV;
    };

    //
    // - - -
    //

    class ThreadPool {
    public:
        explicit ThreadPool(size_t count = [] {
            const unsigned hw = std::thread::hardware_concurrency();
            return std::max(1u, hw > Core::ReservedThreads ? hw - Core::ReservedThreads : 1u);
        }());

        ~ThreadPool();

        ThreadPool(const ThreadPool &) = delete;
        ThreadPool &operator=(const ThreadPool &) = delete;
        ThreadPool(ThreadPool &&) = delete;
        ThreadPool &operator=(ThreadPool &&) = delete;

        void enqueue(Task task);

        void wait();

        void stop();

    private:
        std::vector<std::thread> m_Threads;
        std::queue<Task> m_Tasks;
        std::mutex m_Mutex;
        std::condition_variable m_CV;
        std::atomic<size_t> m_TasksPending{0};
        std::condition_variable m_DoneCV;
        std::atomic<bool> m_ShouldStop{false};
        std::atomic<bool> m_Stopped{false};
    };
} // namespace Platform::Threading
