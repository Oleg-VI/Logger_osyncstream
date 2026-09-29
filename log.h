#pragma once

#include <string>
#include <string_view>
#include <syncstream>   // C++20: std::osyncstream

// -- Log levels ----------------------------------------------------------------
// Closed enum: clients cannot extend this set.
// "using enum" lets main.cpp write DEBUG / INFO / WARNING / ERROR directly.
// Note: on Windows, <windows.h> defines ERROR as 0 -- avoid including it before
// this header, or add "#undef ERROR" prior to including log.h.
enum class LogLevel { DEBUG, INFO, WARNING, ERROR };
using enum LogLevel;

// -- LogMessage ----------------------------------------------------------------
// Represents a single log line being built via operator<<.
//
// DESIGN -- why no mutex/atomic is needed:
//   Each message owns its own std::osyncstream. Everything written through
//   operator<< is accumulated in the osyncstream's private buffer (never
//   shared). When the temporary is destroyed at the end of the full
//   expression, the osyncstream destructor calls emit(), which transfers the
//   whole line to std::cout atomically -- so no two threads can interleave
//   parts of their messages.
//
//   LogMessage is neither copyable nor movable: it is always returned as a
//   prvalue, so C++17 guaranteed copy elision constructs it directly in place.

class LogMessage {
    std::osyncstream out_;

public:
    LogMessage(LogLevel level, std::string_view prefix);

    // Header + first value in one step -- used by Logger::operator<< so it can
    // return a prvalue instead of a named (and therefore moved) local.
    template <typename T>
    LogMessage(LogLevel level, std::string_view prefix, const T& first)
        : LogMessage{level, prefix}
    {
        out_ << first;
    }

    LogMessage(const LogMessage&)            = delete;
    LogMessage& operator=(const LogMessage&) = delete;

    ~LogMessage();

    template <typename T>
    LogMessage& operator<<(const T& val) {
        out_ << val;
        return *this;
    }
};

// -- Logger --------------------------------------------------------------------
// Lightweight handle that carries an optional prefix string.
// Can be obtained via getLogger() or constructed directly: Logger logger{"f2"}.

class Logger {
    std::string prefix_;

public:
    explicit Logger(std::string prefix = "") : prefix_{std::move(prefix)} {}

    // logger(DEBUG) << "msg" -- explicit log level.
    // [[nodiscard]]: a bare "logger(DEBUG);" would print an empty message.
    [[nodiscard]] LogMessage operator()(LogLevel level) const {
        return LogMessage{level, prefix_};
    }

    // logger << "msg" -- default level INFO.
    // Returns the temporary LogMessage so further << chaining works.
    template <typename T>
    LogMessage operator<<(const T& val) const {
        return LogMessage{INFO, prefix_, val};
    }
};

// -- Factory -------------------------------------------------------------------
Logger getLogger(std::string prefix = "");
