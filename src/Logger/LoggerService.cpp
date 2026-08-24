#include "LoggerService.h"

namespace Logging {

    ILogger *LoggerService::s_globalLogger = nullptr;
    thread_local ILogger *LoggerService::s_currentLogger = nullptr;

    void LoggerService::Initialize(ILogger *logger) {
        s_globalLogger = logger;
        s_currentLogger = logger;
    }

    ILogger *LoggerService::Get() { return s_currentLogger ? s_currentLogger : s_globalLogger; }

    bool LoggerService::IsAvailable() { return Get() != nullptr; }

    LoggerService::ScopedOverride::ScopedOverride(ILogger *newLogger) : oldLogger(s_currentLogger) { s_currentLogger = newLogger; }

    LoggerService::ScopedOverride::~ScopedOverride() { s_currentLogger = oldLogger; }

} // namespace Logging
