#include "log.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <thread>

// -- Internal helpers ----------------------------------------------------------
namespace {

std::string_view levelName(LogLevel l) noexcept {
    switch (l) {
    case DEBUG:   return "DEBUG";
    case INFO:    return "INFO";
    case WARNING: return "WARNING";
    case ERROR:   return "ERROR";
    }
    return "UNKNOWN";
}

// Writes the local date and time straight into the target stream.
// std::chrono::current_zone() is not used on purpose: libstdc++ on Windows
// (MinGW, at least GCC 13) cannot detect the system time zone and silently
// falls back to UTC. The C runtime conversion is correct on all platforms.
void writeTimestamp(std::ostream& os) {
    const std::time_t now{std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())};
    std::tm tm{};
#ifdef _WIN32
    ::localtime_s(&tm, &now);
#else
    ::localtime_r(&now, &tm);
#endif
    os << std::put_time(&tm, "%d.%m.%Y %H:%M:%S");
}

} // namespace

// -- LogMessage ----------------------------------------------------------------
LogMessage::LogMessage(LogLevel level, std::string_view prefix)
    : out_{std::cout}
{
    writeTimestamp(out_);
    out_ << "; " << levelName(level) << "; "
         << prefix << '(' << std::this_thread::get_id() << "): ";
}

LogMessage::~LogMessage() {
    // The line is completed here; out_'s destructor then emits it atomically.
    out_ << '\n';
}

// -- Factory -------------------------------------------------------------------
Logger getLogger(std::string prefix) {
    return Logger{std::move(prefix)};
}
