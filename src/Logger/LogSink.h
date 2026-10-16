#pragma once

#include "Logger/ILogSink.h"
#include "Logger/ILogger.h"
#include "Logger/LoggerService.h"
#include <iostream>
#include <mutex>
#include <vector>
namespace Logging {

    class ConsoleSink : public ILogSink {
    public:
        void Write(LogSeverity severity, const std::string &formatted_msg) override {
            if (severity == LogSeverity::Error) {
                std::cerr << formatted_msg;
            } else {
                std::cout << formatted_msg;
            }
        }
    };


    class UILogSink : public ILogSink {
    public:
        void Write(LogSeverity severity, const std::string &formatted_msg) override {
            std::lock_guard lock(mutex_);
            pending_logs_.push_back(formatted_msg);
            pending_logs_severity_.push_back(severity);
        }

        void ConsumePending(std::vector<std::string> &out_logs, std::vector<LogSeverity> &out_severity) {
            std::lock_guard const lock(mutex_);

            if (pending_logs_.empty())
                return;

            out_logs.insert(out_logs.end(), std::make_move_iterator(pending_logs_.begin()), std::make_move_iterator(pending_logs_.end()));
            pending_logs_.clear();
            out_severity.insert(out_severity.end(), std::make_move_iterator(pending_logs_severity_.begin()), std::make_move_iterator(pending_logs_severity_.end()));
            pending_logs_severity_.clear();
        }

    private:
        std::mutex mutex_;
        std::vector<std::string> pending_logs_;
        std::vector<LogSeverity> pending_logs_severity_;
    };


} // namespace Logging
