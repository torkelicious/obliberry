#pragma once
#include "Threading/SmallTask.h"
#include <chrono>
#include <vector>

namespace Platform::Time {

    struct Timer {
        std::chrono::steady_clock::time_point endTime;
        Threading::SmallTask callback; // SmallTask fallbacks to heap alloc function if 2 big so this is fine!!!
    };

    inline std::vector<Timer> timers;

    inline void setTimeout(const std::chrono::milliseconds delay, Threading::SmallTask callback) { timers.push_back({std::chrono::steady_clock::now() + delay, std::move(callback)}); }

    inline void updateTimers() {
        if (timers.empty()) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        for (auto it = timers.begin(); it != timers.end();) {
            if (now >= it->endTime) {
                auto callback = std::move(it->callback);
                it = timers.erase(it);
                callback();
            } else {
                ++it;
            }
        }
    }

} // namespace Platform::Time
