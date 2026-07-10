#pragma once

#include "Logger.h"

namespace Core::Logging {

    class LoggerService {
    public:
        static void Initialize(Logger<1000> *logger);

        static Logger<1000> *Get();

        static bool IsAvailable();

        // Scoped logger override for testing
        class ScopedOverride {
            Logger<1000> *oldLogger;

        public:
            explicit ScopedOverride(Logger<1000> *newLogger);
            ~ScopedOverride();
            ScopedOverride(const ScopedOverride &) = delete;
            ScopedOverride &operator=(const ScopedOverride &) = delete;
        };

    private:
        static thread_local Logger<1000> *s_currentLogger;
    };

#define LOG_INFO(who, msg)                                                                                             \
    do {                                                                                                               \
        if (auto *logger = Core::Logging::LoggerService::Get()) {                                                      \
            logger->log(who, msg, Core::Logging::LogSeverity::Info);                                                   \
        }                                                                                                              \
    } while (0)

#define LOG_WARN(who, msg)                                                                                             \
    do {                                                                                                               \
        if (auto *logger = Core::Logging::LoggerService::Get()) {                                                      \
            logger->log(who, msg, Core::Logging::LogSeverity::Warn);                                                   \
        }                                                                                                              \
    } while (0)

#define LOG_ERROR(who, msg)                                                                                            \
    do {                                                                                                               \
        if (auto *logger = Core::Logging::LoggerService::Get()) {                                                      \
            logger->log(who, msg, Core::Logging::LogSeverity::Error);                                                  \
        }                                                                                                              \
    } while (0)

#define LOG_DEBUG(who, msg)                                                                                            \
    do {                                                                                                               \
        if (auto *logger = Core::Logging::LoggerService::Get()) {                                                      \
            logger->log(who, msg, Core::Logging::LogSeverity::Debug);                                                  \
        }                                                                                                              \
    } while (0)
} // namespace Core::Logging
