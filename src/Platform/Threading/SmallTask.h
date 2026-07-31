#pragma once
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>

namespace Platform::Threading {
    // non heap allocated function wrapper for small lambdas
    // used for renderer InitQ (with exception of lighting system)
    // 24 byte inline storage (for GLinit tasks)
    // Move only , trivially destructible
    // It will fall back to heap if lambda exceeds storage.
    class SmallTask {
        static constexpr size_t StorageSize = 24;
        alignas(8) std::byte m_Storage[StorageSize];
        void (*m_Invoke)(std::byte *) = nullptr;
        bool m_UsesHeap = false;
        void (*m_Destroy)(std::byte *) = nullptr;

    public:
        SmallTask() = default;

        template <typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, SmallTask>>> explicit SmallTask(F &&f) {
            using FuncType = std::decay_t<F>;
            if constexpr (sizeof(FuncType) <= StorageSize && alignof(FuncType) <= alignof(decltype(m_Storage)) && std::is_trivially_move_constructible_v<FuncType> && std::is_trivially_destructible_v<FuncType>) {
                new(m_Storage) FuncType(std::forward<F>(f));
                m_Invoke = [](std::byte *ptr) { (*reinterpret_cast<FuncType *>(ptr))(); };
                m_UsesHeap = false;
                m_Destroy = nullptr;
            } else {
                // heap fallback
                auto *heapFunc = new FuncType(std::forward<F>(f));
                m_Invoke = [](std::byte *ptr) { (*reinterpret_cast<FuncType **>(ptr))->operator()(); };
                *reinterpret_cast<FuncType **>(m_Storage) = heapFunc;
                m_UsesHeap = true;
                m_Destroy = [](std::byte *ptr) { delete *reinterpret_cast<FuncType **>(ptr); };
            }
        }

        SmallTask(SmallTask &&other) noexcept : m_Invoke(other.m_Invoke), m_UsesHeap(other.m_UsesHeap), m_Destroy(other.m_Destroy) {
            if (m_Invoke) {
                if (m_UsesHeap) {
                    *reinterpret_cast<void **>(m_Storage) = *reinterpret_cast<void **>(other.m_Storage);
                } else {
                    std::memcpy(m_Storage, other.m_Storage, StorageSize);
                }
                other.m_Invoke = nullptr;
                other.m_UsesHeap = false;
                other.m_Destroy = nullptr;
            }
        }

        SmallTask &operator=(SmallTask &&other) noexcept {
            if (this != &other) {
                if (m_Destroy) {
                    m_Destroy(m_Storage);
                }
                m_Invoke = other.m_Invoke;
                m_UsesHeap = other.m_UsesHeap;
                m_Destroy = other.m_Destroy;
                if (m_Invoke) {
                    if (m_UsesHeap) {
                        *reinterpret_cast<void **>(m_Storage) = *reinterpret_cast<void **>(other.m_Storage);
                    } else {
                        std::memcpy(m_Storage, other.m_Storage, StorageSize);
                    }
                    other.m_Invoke = nullptr;
                    other.m_UsesHeap = false;
                    other.m_Destroy = nullptr;
                }
            }
            return *this;
        }

        ~SmallTask() {
            if (m_Destroy) {
                m_Destroy(m_Storage);
            }
        }

        SmallTask(const SmallTask &) = delete;
        SmallTask &operator=(const SmallTask &) = delete;

        void operator()() {
            if (m_Invoke) {
                m_Invoke(m_Storage);
            }
        }

        explicit operator bool() const { return m_Invoke != nullptr; }
        [[nodiscard]] bool UsesHeap() const { return m_UsesHeap; }
    };
} // namespace Platform::Threading
