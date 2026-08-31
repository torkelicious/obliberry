#pragma once
#include <algorithm>
#include <optional>
#include <vector>

namespace Core::Utils {

    namespace vector {
        // move item from one index to another inside an std::vector
        template <typename T> void moveItem(std::vector<T> &vec, std::size_t from, size_t to) {
            if (from >= vec.size() || to >= vec.size() || from == to)
                return;

            if (from < to) {
                std::rotate(vec.begin() + from, vec.begin() + from + 1, vec.begin() + to + 1);
            } else {
                std::rotate(vec.begin() + to, vec.begin() + from, vec.begin() + from + 1);
            }
        }

        // get index of T in vector vec
        template <typename T> std::optional<std::size_t> getIndex(const std::vector<T> &vec, const T &item) {
            auto it = std::find(vec.begin(), vec.end(), item);

            if (it == vec.end())
                return std::nullopt;

            return std::distance(vec.begin(), it);
        }

        // overload which auto-deduces 'from'
        template <typename T> void moveItem(std::vector<T> &vec, const T &item, size_t to) {
            if (auto idx = getIndex(vec, item)) {
                moveItem(vec, *idx, to);
            }
        }
    } // namespace vector


} // namespace Core::Utils
