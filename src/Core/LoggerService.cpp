#include "LoggerService.h"

namespace Core::Logging {

    thread_local ILogger *LoggerService::s_currentLogger = nullptr;

    void LoggerService::Initialize(ILogger *logger) { s_currentLogger = logger; }

    ILogger *LoggerService::Get() { return s_currentLogger; }

    bool LoggerService::IsAvailable() { return s_currentLogger != nullptr; }

    LoggerService::ScopedOverride::ScopedOverride(ILogger *newLogger) : oldLogger(s_currentLogger) {
        s_currentLogger = newLogger;
    }

    LoggerService::ScopedOverride::~ScopedOverride() { s_currentLogger = oldLogger; }

} // namespace Core::Logging
