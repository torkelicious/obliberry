#pragma once
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>

namespace Platform::Threading {
    // non-heap allocated function wrapper
    // type-erased
    // (56-byte aligned stack buffer)
    class Task {
        static constexpr size_t StorageSize = 56;
        alignas(64) std::byte m_Storage[StorageSize];
        void (*m_Invoke)(std::byte *) = nullptr;

    public:
        Task() = default;

        template <typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, Task>>> explicit Task(F &&f) {
            using FuncType = std::decay_t<F>;
            static_assert(sizeof(FuncType) <= StorageSize, "Task lambda is too large for inline storage!");
            static_assert(std::is_trivially_move_constructible_v<FuncType>, "Task lambda must be trivially movable!");
            static_assert(std::is_trivially_destructible_v<FuncType>, "Task lambda must be trivially destructible!");

            new(m_Storage) FuncType(std::forward<F>(f));
            m_Invoke = [](std::byte *ptr) { (*reinterpret_cast<FuncType *>(ptr))(); };
        }

        Task(Task &&other) noexcept : m_Invoke(other.m_Invoke) {
            if (m_Invoke) {
                std::memcpy(m_Storage, other.m_Storage, StorageSize);
                other.m_Invoke = nullptr;
            }
        }

        Task &operator=(Task &&other) noexcept {
            m_Invoke = other.m_Invoke;
            if (m_Invoke) {
                std::memcpy(m_Storage, other.m_Storage, StorageSize);
                other.m_Invoke = nullptr;
            }
            return *this;
        }

        Task(const Task &) = delete;
        Task &operator=(const Task &) = delete;

        void operator()() {
            if (m_Invoke)
                m_Invoke(m_Storage);
        }

        explicit operator bool() const { return m_Invoke != nullptr; }
    };
} // namespace Platform::Threading
