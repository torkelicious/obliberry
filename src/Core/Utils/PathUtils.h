#pragma once

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
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
#elif defined(__APPLE__)
        // macOS has no /proc/self/exe; _NSGetExecutablePath gives the path of the
        // running binary, and weakly_canonical resolves symlinks like /proc does.
        uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);
        std::string buffer(size, '\0');
        _NSGetExecutablePath(buffer.data(), &size);
        return std::filesystem::weakly_canonical(buffer).parent_path();
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

    inline std::filesystem::path GetInternalDir() { return GetExecutableDirectory() / "internal"; }

    inline std::filesystem::path GetInternalRecourcesDir() { return GetInternalDir() / "resources"; }

    inline std::filesystem::path GetDataHome() {
#ifdef _WIN32
        PWSTR raw = nullptr;

        const HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &raw);

        if (FAILED(result) || raw == nullptr) {
            if (raw) {
                CoTaskMemFree(raw);
            }

            throw std::runtime_error("Could not locate the Windows LocalAppData directory");
        }

        std::filesystem::path path(raw);
        CoTaskMemFree(raw);

        return path;

#elif defined(__APPLE__)
        const auto home = GetHomeDirectory();

        if (home.empty()) {
            throw std::runtime_error("Could not locate the macOS home directory");
        }

        return home / "Library" / "Application Support";

#else
        if (const char *xdgDataHome = std::getenv("XDG_DATA_HOME"); xdgDataHome && *xdgDataHome != '\0') {

            std::filesystem::path path(xdgDataHome);

            if (path.is_absolute()) {
                return path;
            }
        }

        const auto home = GetHomeDirectory();

        if (home.empty()) {
            throw std::runtime_error("Could not locate the Linux home directory");
        }

        return home / ".local" / "share";
#endif
    }


} // namespace Core::PathUtils
