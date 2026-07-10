#include "LoggerService.h"

namespace Core::Logging {

    thread_local Logger<1000> *LoggerService::s_currentLogger = nullptr;

    void LoggerService::Initialize(Logger<1000> *logger) { s_currentLogger = logger; }

    Logger<1000> *LoggerService::Get() { return s_currentLogger; }

    bool LoggerService::IsAvailable() { return s_currentLogger != nullptr; }

    LoggerService::ScopedOverride::ScopedOverride(Logger<1000> *newLogger) : oldLogger(s_currentLogger) {
        s_currentLogger = newLogger;
    }

    LoggerService::ScopedOverride::~ScopedOverride() { s_currentLogger = oldLogger; }

} // namespace Core::Logging
