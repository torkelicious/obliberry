#include <chrono>
#include <ctime>
#include <filesystem>
#include <string>

namespace Core::Utils::Fmt {

    inline std::string formatFileTimestamp(std::filesystem::file_time_type time) {
        const auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(time);

        const std::time_t timeT = std::chrono::system_clock::to_time_t(systemTime);

        std::tm tm{};

#ifdef _WIN32
        localtime_s(&tm, &timeT);
#else
        localtime_r(&timeT, &tm);
#endif

        char buffer[64]{};
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);

        return buffer;
    }

} // namespace Core::Utils::Fmt
