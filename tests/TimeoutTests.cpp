#include "Platform/Timeout.h"
#include <gtest/gtest.h>
#include <atomic>
#include <barrier>
#include <thread>
#include <utility>

class TimeoutTests : public testing::Test {
protected:
    template <typename F> static void Schedule(F &&callback) {
        Platform::Time::setTimeout(std::chrono::milliseconds{0}, Platform::Threading::SmallTask{std::forward<F>(callback)});
    }
    void SetUp() override { Clear(); }
    void TearDown() override { Clear(); }
    static void Clear() {
        std::lock_guard lock(Platform::Time::timersMutex);
        Platform::Time::timers.clear();
        ++Platform::Time::currentGeneration;
    }
};

TEST_F(TimeoutTests, DueCallbacksRunExactlyOnce) {
    int calls = 0;
    Schedule([&] { ++calls; });
    Platform::Time::updateTimers();
    Platform::Time::updateTimers();
    EXPECT_EQ(calls, 1);
}

TEST_F(TimeoutTests, CallbackCanScheduleAnotherTimerWithoutDeadlocking) {
    int calls = 0;
    Schedule([&] {
        ++calls;
        Schedule([&] { ++calls; });
    });
    Platform::Time::updateTimers();
    EXPECT_EQ(calls, 1);
    Platform::Time::updateTimers();
    EXPECT_EQ(calls, 2);
}

TEST_F(TimeoutTests, InvalidatingGenerationSkipsOldTimersButRunsNewTimers) {
    int oldCalls = 0;
    int newCalls = 0;
    Schedule([&] { ++oldCalls; });
    Platform::Time::invalidateGeneration();
    Schedule([&] { ++newCalls; });
    Platform::Time::updateTimers();
    EXPECT_EQ(oldCalls, 0);
    EXPECT_EQ(newCalls, 1);
}

TEST_F(TimeoutTests, CallbackInvalidationSkipsRemainingCallbacksFromSameBatch) {
    int firstCalls = 0;
    int staleCalls = 0;
    Schedule([&] {
        ++firstCalls;
        Platform::Time::invalidateGeneration();
    });
    Schedule([&] { ++staleCalls; });
    Platform::Time::updateTimers();
    EXPECT_EQ(firstCalls, 1);
    EXPECT_EQ(staleCalls, 0);
}

TEST_F(TimeoutTests, ConcurrentSchedulingAndPollingDoesNotLoseOrDuplicateCallbacks) {
    constexpr int Count = 500;
    std::atomic<int> calls{0};
    std::atomic<bool> finished{false};
    std::barrier start{2};
    std::jthread producer([&] {
        start.arrive_and_wait();
        for (int i = 0; i < Count; ++i) {
            Schedule([&] { ++calls; });
        }
        finished.store(true);
    });
    start.arrive_and_wait();
    while (!finished.load()) {
        Platform::Time::updateTimers();
        std::this_thread::yield();
    }
    producer.join();
    Platform::Time::updateTimers();
    EXPECT_EQ(calls, Count);
}
