#pragma once
#include "Threading/SmallTask.h"
#include <chrono>
#include <cstdint>
#include <mutex>
#include <vector>

namespace Platform::Time {

    inline uint64_t currentGeneration = 0;

    struct Timer {
        std::chrono::steady_clock::time_point endTime;
        Threading::SmallTask callback; // SmallTask fallbacks to heap alloc function if 2 big so this is fine!!!
        uint64_t generation = 0;
    };

    inline std::vector<Timer> timers;
    inline std::mutex timersMutex;

    inline void setTimeout(const std::chrono::milliseconds delay, Threading::SmallTask callback) {
        std::lock_guard lock(timersMutex);
        timers.push_back({.endTime = std::chrono::steady_clock::now() + delay, .callback = std::move(callback), .generation = currentGeneration});
    }

    inline void invalidateGeneration() {
        std::lock_guard lock(timersMutex);
        ++currentGeneration;
    }

    inline void updateTimers() {
        std::vector<Timer> due;

        {
            std::lock_guard lock(timersMutex);
            const auto now = std::chrono::steady_clock::now();

            for (auto it = timers.begin(); it != timers.end();) {
                if (now >= it->endTime) {
                    due.push_back(std::move(*it));
                    it = timers.erase(it);
                } else {
                    ++it;
                }
            }
        }

        for (auto &timer : due) {
            {
                std::lock_guard lock(timersMutex);
                if (timer.generation != currentGeneration) {
                    continue;
                }
            }
            timer.callback();
        }
    }

} // namespace Platform::Time
