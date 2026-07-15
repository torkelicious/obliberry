#pragma once

#include <string>
#include <cstdint>

namespace Logging {

    enum class LogSeverity : uint8_t { Debug, Info, Warn, Error };

    class ILogger {
    public:
        virtual ~ILogger() = default;
        virtual void log(std::string who, std::string what, LogSeverity severity = LogSeverity::Info) = 0;
    };

} // namespace Logging
