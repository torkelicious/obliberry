#pragma once

#include <filesystem>
#include <string_view>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <sys/types.h>
#include <unistd.h>
#endif

namespace Core::Utils::OS {

#ifdef _WIN32

    static bool OsOpen(const wchar_t *target) {
        HINSTANCE result = ShellExecuteW(nullptr, L"open", target, nullptr, nullptr, SW_SHOWNORMAL);

        return reinterpret_cast<INT_PTR>(result) > 32;
    }

#else

    static bool OsOpen(const char *target) {
        const pid_t pid = fork();
        if (pid == -1)
            return false;

        if (pid == 0) {
#ifdef __APPLE__
            execlp("open", "open", target, nullptr);
#else
            execlp("xdg-open", "xdg-open", target, nullptr);
#endif
            _exit(127);
        }
        return true;
    }

#endif

    static bool OsOpenFile(const std::filesystem::path &path) { return OsOpen(path.c_str()); }

    static bool OsOpenUrl(std::string_view url) {
#ifdef _WIN32
        const std::wstring wurl = Utf8ToUtf16(url);
        return OsOpen(wurl.c_str());
#else
        return OsOpen(url.data());
#endif
    }

} // namespace Core::Utils::OS
