#pragma once
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif


namespace Core::Utils::Scripting {
    static bool OsOpenFile(const std::filesystem::path &path) {
#ifdef _WIN32
        HINSTANCE result = ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return reinterpret_cast<std::intptr_t>(result) > 32;
#else
        const pid_t pid = fork();
        if (pid == -1)
            return false;

        if (pid == 0) {
#ifdef __APPLE__
            execlp("open", "open", path.c_str(), nullptr);
#else
            execlp("xdg-open", "xdg-open", path.c_str(), nullptr);
#endif
            _exit(127);
        }
        return true;
#endif
    }

} // namespace Core::Utils::Scripting
