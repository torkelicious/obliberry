#pragma once
#include "Threading/SmallTask.h"
#include <chrono>
#include <cstdint>
#include <vector>

namespace Platform::Time {

    inline uint64_t currentGeneration = 0;

    struct Timer {
        std::chrono::steady_clock::time_point endTime;
        Threading::SmallTask callback; // SmallTask fallbacks to heap alloc function if 2 big so this is fine!!!
        uint64_t generation = 0;
    };

    inline std::vector<Timer> timers;

    inline void setTimeout(const std::chrono::milliseconds delay, Threading::SmallTask callback) {
        timers.push_back({.endTime = std::chrono::steady_clock::now() + delay, .callback = std::move(callback), .generation = currentGeneration});
    }

    inline void invalidateGeneration() { ++currentGeneration; }

    inline void updateTimers() {
        if (timers.empty()) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();

        std::vector<Timer> due;
        for (auto it = timers.begin(); it != timers.end();) {
            if (now >= it->endTime) {
                due.push_back(std::move(*it));
                it = timers.erase(it);
            } else {
                ++it;
            }
        }

        for (auto &timer : due) {
            if (timer.generation != currentGeneration) {
                continue;
            }
            timer.callback();
        }
    }

} // namespace Platform::Time
