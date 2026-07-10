#pragma once
#include <filesystem>
#include <glad/glad.h>
#include <imgui.h>
#include <optional>
#include <string>

namespace Core::PathUtils {
    // accepts any number of string_views and joins them
    inline std::string Join(const std::string_view p1, const std::string_view p2, const std::string_view p3 = "") {
        std::string result;
        result.reserve(p1.size() + p2.size() + p3.size());
        result += p1;
        result += p2;
        result += p3;
        return result;
    }

    // returns the directory containing the running executable
    inline std::filesystem::path GetExecutableDirectory() {
#ifdef _WIN32
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        return std::filesystem::path(path).parent_path();
#else
        return std::filesystem::canonical("/proc/self/exe").parent_path();
#endif
    }
} // namespace Core::PathUtils


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

namespace Core::Utils::UI {
    // textures are loaded with stbi flip on so they must be unflipped
    static void ImGuiImageFlipped(const GLuint textureID, const ImVec2 &size) {
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        drawList->AddImage(textureID, cursorPos, ImVec2(cursorPos.x + size.x, cursorPos.y + size.y), ImVec2(0, 1),
                           ImVec2(1, 0));
        ImGui::Dummy(size);
    }
} // namespace Core::Utils::UI
