// core/log.cpp

#include "log.hpp"
#include <cstdarg>
#include <cstdio>
#include <ctime>

namespace core {

namespace {
    LogLevel g_minLevel = LogLevel::Debug;

    const char* LevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info:  return "INFO ";
            case LogLevel::Warn:  return "WARN ";
            case LogLevel::Error: return "ERROR";
        }
        return "?????";
    }

    // ANSI colors — harmless on terminals that don't support them,
    // they just show as a couple of stray characters.
    const char* LevelToColor(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return "\033[90m"; // gray
            case LogLevel::Info:  return "\033[37m"; // white
            case LogLevel::Warn:  return "\033[33m"; // yellow
            case LogLevel::Error: return "\033[31m"; // red
        }
        return "\033[0m";
    }
}

void Log::SetMinLevel(LogLevel level) {
    g_minLevel = level;
}

void Log::Write(LogLevel level, const char* category, const char* fmt, va_list args) {
    if (level < g_minLevel) return;

    std::time_t t = std::time(nullptr);
    std::tm tmv{};
#if defined(_WIN32)
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif

    char timeBuf[16];
    std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tmv);

    std::fprintf(stderr, "%s[%s] [%s] [%-10s] ",
                 LevelToColor(level), timeBuf, LevelToString(level), category);

    std::vfprintf(stderr, fmt, args);

    std::fprintf(stderr, "\033[0m\n");
}

void Log::Debug(const char* category, const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    Write(LogLevel::Debug, category, fmt, args);
    va_end(args);
}

void Log::Info(const char* category, const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    Write(LogLevel::Info, category, fmt, args);
    va_end(args);
}

void Log::Warn(const char* category, const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    Write(LogLevel::Warn, category, fmt, args);
    va_end(args);
}

void Log::Error(const char* category, const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    Write(LogLevel::Error, category, fmt, args);
    va_end(args);
}

} // namespace core
