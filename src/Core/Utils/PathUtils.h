#pragma once
#include <filesystem>
#ifdef _WIN32
    #include <windows.h>
#endif

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


    inline std::filesystem::path GetHomeDirectory() {
#ifdef _WIN32
        const char *path = std::getenv("USERPROFILE");
#else
        const char *path = std::getenv("HOME");
#endif
        return path ? std::filesystem::path(path) : std::filesystem::path{};
    }

} // namespace Core::PathUtils
