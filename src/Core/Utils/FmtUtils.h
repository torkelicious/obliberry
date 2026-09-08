#pragma once

#include <chrono>
#include <ctime>
#include <filesystem>
#include <string>

namespace Core::Utils::Fmt {

    inline std::string formatFileTimestamp(std::filesystem::file_time_type time) {
        const auto fileNow = std::filesystem::file_time_type::clock::now();
        const auto sysNow = std::chrono::system_clock::now();
        const auto sysTime = sysNow + std::chrono::duration_cast<std::chrono::system_clock::duration>(time - fileNow);

        const std::time_t timeT = std::chrono::system_clock::to_time_t(sysTime);
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &timeT);
#else
        localtime_r(&timeT, &tm);
#endif
        char buffer[64]{};
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);

        return buffer;
    }

} // namespace Core::Utils::Fmt
