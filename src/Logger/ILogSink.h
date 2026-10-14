#pragma once

#include "Logger/ILogger.h"

namespace Logging {

    class ILogSink {
    public:
        virtual ~ILogSink() = default;
        virtual void Write(LogSeverity severity, const std::string &formatted_msg) = 0;
    };


} // namespace Logging
