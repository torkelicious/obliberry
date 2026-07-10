#pragma once

#include "ILogger.h"
#include "Utils.h"
#include <string>
#include <filesystem>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <thread>
#include <iostream>
#include <utility>
#include <format>
#include <chrono>

namespace Core::Logging {

    // to format the raw time_point into a readable string
    inline std::string getTimestamp(std::chrono::system_clock::time_point time) {
        const auto local_time = std::chrono::current_zone()->to_local(time);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()) % 1000;
        return std::format("{:%Y-%m-%d %H:%M:%S}.{:03}", std::chrono::floor<std::chrono::seconds>(local_time),
                           ms.count());
    }

    struct Log {
        std::string who;
        std::string what;
        std::chrono::system_clock::time_point timestamp;
        LogSeverity severity;
    };

    template <std::size_t QueueSize> class Logger : public ILogger {
    public:
        explicit Logger(const std::filesystem::path &logfile_path) {
            logfile_.open(logfile_path, std::ios::out | std::ios::app);
            init();
        }

        Logger() { init(); }

        ~Logger() override { cv_.notify_all(); }

        void log(std::string who, std::string what, LogSeverity severity = LogSeverity::Info) override {
            const auto now = std::chrono::system_clock::now();
            std::lock_guard lock(mutex_);
            buffer_.push(Log{std::move(who), std::move(what), now, severity});
            cv_.notify_one();
        }

    private:
        void init() {
            worker_ = std::jthread([this](const std::stop_token &stop_token) { LoggerLoop(stop_token); });
        }

        void LoggerLoop(std::stop_token stop_token) {
            while (true) {
                std::unique_lock lock(mutex_);
                cv_.wait(lock, stop_token, [this] { return !buffer_.empty(); });
                while (!buffer_.empty()) {
                    if (auto log_item = buffer_.pop()) {
                        std::string prefix;

                        switch (log_item->severity) {
                            case LogSeverity::Debug:
#if !DEBUG_BUILD
                                continue;
#endif
                                prefix = "DEBUG: ";
                                break;
                            case LogSeverity::Info:
                                prefix = "INFO:  ";
                                break;
                            case LogSeverity::Warn:
                                prefix = "WARN:  ";
                                break;
                            case LogSeverity::Error:
                                prefix = "ERROR: ";
                                break;
                        }
                        std::string ts = getTimestamp(log_item->timestamp);
                        std::string logfmt = std::format("[{}] {}[{}] {}\n", ts, prefix, log_item->who, log_item->what);
                        if (logfile_.is_open()) {
                            logfile_ << logfmt;
                            logfile_.flush();
                        }
                        if (log_item->severity == LogSeverity::Error) {
                            std::cerr << logfmt;
                        } else {
                            std::cout << logfmt;
                        }
                    }
                }
                if (stop_token.stop_requested() && buffer_.empty()) {
                    break;
                }
            }
        }
        std::fstream logfile_;
        Utils::CircularBuffer<Log, QueueSize> buffer_;
        std::mutex mutex_;
        std::condition_variable_any cv_;
        std::jthread worker_;
    };

} // namespace Core::Logging
