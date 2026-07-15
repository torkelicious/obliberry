#pragma once

#include "ILogger.h"

namespace Logging {

    class LoggerService {
    public:
        static void Initialize(ILogger *logger);

        static ILogger *Get();

        static bool IsAvailable();

        class ScopedOverride {
            ILogger *oldLogger;

        public:
            explicit ScopedOverride(ILogger *newLogger);
            ~ScopedOverride();
            ScopedOverride(const ScopedOverride &) = delete;
            ScopedOverride &operator=(const ScopedOverride &) = delete;
        };

    private:
        static thread_local ILogger *s_currentLogger;
    };

#define LOG_INFO(who, msg)                                                                                                                                                                                                 \
    do {                                                                                                                                                                                                                   \
        if (auto *logger = Logging::LoggerService::Get()) {                                                                                                                                                                \
            logger->log(who, msg, Logging::LogSeverity::Info);                                                                                                                                                             \
        }                                                                                                                                                                                                                  \
    } while (0)

#define LOG_WARN(who, msg)                                                                                                                                                                                                 \
    do {                                                                                                                                                                                                                   \
        if (auto *logger = Logging::LoggerService::Get()) {                                                                                                                                                                \
            logger->log(who, msg, Logging::LogSeverity::Warn);                                                                                                                                                             \
        }                                                                                                                                                                                                                  \
    } while (0)

#define LOG_ERROR(who, msg)                                                                                                                                                                                                \
    do {                                                                                                                                                                                                                   \
        if (auto *logger = Logging::LoggerService::Get()) {                                                                                                                                                                \
            logger->log(who, msg, Logging::LogSeverity::Error);                                                                                                                                                            \
        }                                                                                                                                                                                                                  \
    } while (0)

#define LOG_DEBUG(who, msg)                                                                                                                                                                                                \
    do {                                                                                                                                                                                                                   \
        if (auto *logger = Logging::LoggerService::Get()) {                                                                                                                                                                \
            logger->log(who, msg, Logging::LogSeverity::Debug);                                                                                                                                                            \
        }                                                                                                                                                                                                                  \
    } while (0)
} // namespace Logging
