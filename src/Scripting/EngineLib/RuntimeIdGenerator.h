#pragma once

#include <atomic>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace Scripting {
    class RuntimeIdGenerator {
    public:
        [[nodiscard]] static uint64_t Generate() {
            const uint64_t id = next_.fetch_add(1, std::memory_order_relaxed);
            if (id == 0 || id == std::numeric_limits<uint64_t>::max()) {
                throw std::overflow_error("scripting object id space exhausted");
            }
            return id;
        }

    protected:
        inline static std::atomic<uint64_t> next_{1};
    };

} // namespace Scripting
