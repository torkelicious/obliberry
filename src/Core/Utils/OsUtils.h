#pragma once

#include <filesystem>
#include <limits>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#endif

namespace Core::Utils::OS {

#ifdef _WIN32

    inline std::wstring Utf8ToUtf16(std::string_view utf8) {
        if (utf8.empty())
            return {};

        if (utf8.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
            return {};

        const int inputSize = static_cast<int>(utf8.size());

        const int outputSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), inputSize, nullptr, 0);

        if (outputSize <= 0)
            return {};

        std::wstring result(static_cast<size_t>(outputSize), L'\0');

        const int converted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), inputSize, result.data(), outputSize);

        if (converted != outputSize)
            return {};

        return result;
    }

    inline bool OsOpen(const wchar_t *target) {
        if (!target || !*target)
            return false;

        // default action
        HINSTANCE result = ShellExecuteW(nullptr, nullptr, target, nullptr, nullptr, SW_SHOWNORMAL);

        // (on error 31) show "Open With" dialog
        if (reinterpret_cast<INT_PTR>(result) == SE_ERR_NOASSOC) {
            result = ShellExecuteW(nullptr, L"openas", target, nullptr, nullptr, SW_SHOWNORMAL);
        }
        return reinterpret_cast<INT_PTR>(result) > 32;
    }

#else

    inline bool OsOpen(const char *target) {
        if (!target || !*target)
            return false;

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

    inline bool OsOpenFile(const std::filesystem::path &path) {
#ifdef _WIN32
        std::filesystem::path nativePath = path;
        nativePath.make_preferred();
        return OsOpen(nativePath.c_str());
#else
        const std::string nativePath = path.string();
        return OsOpen(nativePath.c_str());
#endif
    }

    inline bool OsOpenUrl(const std::string_view url) {
        if (url.empty())
            return false;

#ifdef _WIN32
        const std::wstring utf16Url = Utf8ToUtf16(url);

        if (utf16Url.empty())
            return false;

        return OsOpen(utf16Url.c_str());
#else
        const std::string urlString(url);
        return OsOpen(urlString.c_str());
#endif
    }

} // namespace Core::Utils::OS
