// core/log.hpp
//
// Minimal leveled logger. Deliberately simple — no async queue, no
// file rotation. Build that later once you actually feel the need
// for it (you won't, for a long time).

#pragma once

#include "types.hpp"
#include <string>

namespace core {

enum class LogLevel : u8 {
    Debug = 0,
    Info  = 1,
    Warn  = 2,
    Error = 3,
};

class Log {
public:
    // Sets the minimum level that will actually print. Anything
    // below this is silently dropped — cheap, since we check before
    // formatting anything.
    static void SetMinLevel(LogLevel level);

    static void Debug(const char* category, const char* fmt, ...);
    static void Info(const char* category, const char* fmt, ...);
    static void Warn(const char* category, const char* fmt, ...);
    static void Error(const char* category, const char* fmt, ...);

private:
    static void Write(LogLevel level, const char* category, const char* fmt, va_list args);
};

} // namespace core

// Convenience macros — use these instead of calling core::Log directly,
// so call sites stay short: LOG_INFO("Render", "Loaded %d textures", n);
#define LOG_DEBUG(category, ...) core::Log::Debug(category, __VA_ARGS__)
#define LOG_INFO(category, ...)  core::Log::Info(category, __VA_ARGS__)
#define LOG_WARN(category, ...)  core::Log::Warn(category, __VA_ARGS__)
#define LOG_ERROR(category, ...) core::Log::Error(category, __VA_ARGS__)
