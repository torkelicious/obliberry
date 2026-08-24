#pragma once
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace Scripting {
    // move only, small buffer replacement for std::function<void(Args...)>.
    //
    // unlike Platform::Threading::SmallTask/Task this does not require the wrapped
    // callable to be trivially copyable/destructible
    // command buffer lambdas can
    // capture things like by move.
    //
    // callables that don't fit inline fall back to a heap allocation
    //
    // supposed to be  drop in replacement for std::function<void(Args...)> as a
    // member/parameter type
    // construction from a lambda is implicit

    template <typename Signature> class SmallFunction;

    template <typename... Args> class SmallFunction<void(Args...)> {
        // tune upward s if shows frequent heap fallback
        static constexpr size_t StorageSize = 32;
        alignas(alignof(std::max_align_t)) std::byte m_Storage[StorageSize]{};

        void (*m_Invoke)(std::byte *, Args...) = nullptr;
        void (*m_MoveCtor)(std::byte *dst, std::byte *src) = nullptr;
        void (*m_Destroy)(std::byte *) = nullptr;
        bool m_UsesHeap = false;

        template <typename F> static constexpr bool FitsInline = sizeof(F) <= StorageSize && alignof(F) <= alignof(std::max_align_t);

    public:
        SmallFunction() = default;

        // ReSharper disable once CppNonExplicitConvertingConstructor
        template <typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, SmallFunction>>> SmallFunction(F &&f) { // NOLINT(*-explicit-constructor)
            using FuncType = std::decay_t<F>;
            if constexpr (FitsInline<FuncType>) {
                new (static_cast<void *>(m_Storage)) FuncType(std::forward<F>(f));
                m_Invoke = [](std::byte *ptr, Args... args) { (*std::launder(reinterpret_cast<FuncType *>(ptr)))(std::forward<Args>(args)...); };
                m_Destroy = [](std::byte *ptr) { std::launder(reinterpret_cast<FuncType *>(ptr))->~FuncType(); };
                m_MoveCtor = [](std::byte *dst, std::byte *src) {
                    auto *srcObj = std::launder(reinterpret_cast<FuncType *>(src));
                    new (static_cast<void *>(dst)) FuncType(std::move(*srcObj));
                    srcObj->~FuncType();
                };
                m_UsesHeap = false;
            } else {
                auto *heapFunc = new FuncType(std::forward<F>(f));
                *reinterpret_cast<FuncType **>(m_Storage) = heapFunc;
                m_Invoke = [](std::byte *ptr, Args... args) { (**reinterpret_cast<FuncType **>(ptr))(std::forward<Args>(args)...); };
                m_Destroy = [](std::byte *ptr) { delete *reinterpret_cast<FuncType **>(ptr); };
                m_MoveCtor = [](std::byte *dst, std::byte *src) {
                    *reinterpret_cast<FuncType **>(dst) = *reinterpret_cast<FuncType **>(src);
                    *reinterpret_cast<FuncType **>(src) = nullptr;
                };
                m_UsesHeap = true;
            }
        }

        SmallFunction(SmallFunction &&other) noexcept { moveFrom(other); }

        SmallFunction &operator=(SmallFunction &&other) noexcept {
            if (this != &other) {
                reset();
                moveFrom(other);
            }
            return *this;
        }

        SmallFunction(const SmallFunction &) = delete;
        SmallFunction &operator=(const SmallFunction &) = delete;

        ~SmallFunction() { reset(); }

        void operator()(Args... args) {
            if (m_Invoke)
                m_Invoke(m_Storage, std::forward<Args>(args)...);
        }

        explicit operator bool() const { return m_Invoke != nullptr; }
        [[nodiscard]] bool UsesHeap() const { return m_UsesHeap; }

    private:
        void moveFrom(SmallFunction &other) noexcept {
            m_Invoke = other.m_Invoke;
            m_Destroy = other.m_Destroy;
            m_MoveCtor = other.m_MoveCtor;
            m_UsesHeap = other.m_UsesHeap;
            if (m_MoveCtor)
                m_MoveCtor(m_Storage, other.m_Storage);
            other.m_Invoke = nullptr;
            other.m_Destroy = nullptr;
            other.m_MoveCtor = nullptr;
            other.m_UsesHeap = false;
        }

        void reset() noexcept {
            if (m_Destroy)
                m_Destroy(m_Storage);
            m_Invoke = nullptr;
            m_Destroy = nullptr;
            m_MoveCtor = nullptr;
            m_UsesHeap = false;
        }
    };
} // namespace Scripting


// cpp23: https://en.cppreference.com/cpp/utility/functional/move_only_function
// https://stackoverflow.com/questions/25330716/move-only-version-of-stdfunction
