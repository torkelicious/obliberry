#pragma once

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
} // namespace Core::PathUtils
