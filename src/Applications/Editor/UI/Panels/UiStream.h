#pragma once

#include "Logger/LogSink.h"
#include <memory>
#include <ostream>
#include <streambuf>
namespace Editor::UI {

    class UIStreamBuf : public std::streambuf {
    public:
        explicit UIStreamBuf(std::shared_ptr<Logging::UILogSink> sink) : sink_(std::move(sink)) {}

    protected:
        int_type overflow(int_type ch) override {
            if (ch != traits_type::eof()) {
                char const c = static_cast<char>(ch);
                if (c == '\n') {
                    FlushLine();
                } else {
                    lineBuffer_ += c;
                }
            }
            return ch;
        }

        std::streamsize xsputn(const char *s, std::streamsize count) override {
            for (std::streamsize i = 0; i < count; ++i) {
                if (s[i] == '\n') {
                    FlushLine();
                } else {
                    lineBuffer_ += s[i];
                }
            }
            return count;
        }

        int sync() override {
            if (!lineBuffer_.empty()) {
                FlushLine();
            }
            return 0;
        }

    private:
        void FlushLine() {
            if (sink_) {
                sink_->Write(Logging::LogSeverity::Info, lineBuffer_ + "\n");
            }
            lineBuffer_.clear();
        }

        std::shared_ptr<Logging::UILogSink> sink_;
        std::string lineBuffer_;
    };

    class UiStream : public std::ostream {
    public:
        explicit UiStream(std::shared_ptr<Logging::UILogSink> sink) : std::ostream(&buf_), buf_(std::move(sink)) {}
    private:
        UIStreamBuf buf_;
    };


} // namespace Editor::UI
