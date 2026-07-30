#pragma once
#include <filesystem>
#include <optional>
#include <array>

namespace Core::Utils {
    template <std::movable T, std::size_t Size>
        requires(Size > 0)
    class CircularBuffer {
    public:
        constexpr CircularBuffer() = default;

        // adds an element
        // overwriting the oldest data if full
        constexpr void push(T item) noexcept {
            data_[head_] = std::move(item);

            if (full_) {
                tail_ = (tail_ + 1) % Size;
            }

            head_ = (head_ + 1) % Size;
            full_ = head_ == tail_;
        }

        // pop the oldest element
        constexpr std::optional<T> pop() noexcept {
            if (empty()) {
                return std::nullopt;
            }

            full_ = false;
            T item = std::move(data_[tail_]);
            tail_ = (tail_ + 1) % Size;
            return item;
        }

        [[nodiscard]] constexpr bool empty() const noexcept { return !full_ && head_ == tail_; }

        [[nodiscard]] constexpr bool full() const noexcept { return full_; }

        [[nodiscard]] constexpr std::size_t capacity() const noexcept { return Size; }

        [[nodiscard]] constexpr std::size_t size() const noexcept {
            if (full_)
                return Size;
            if (head_ >= tail_)
                return head_ - tail_;
            return Size + head_ - tail_;
        }

    private:
        std::array<T, Size> data_{};
        std::size_t head_ = 0;
        std::size_t tail_ = 0;
        bool full_ = false;
    };
} // namespace Core::Utils
