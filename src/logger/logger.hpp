// Hamster kernel logger

#pragma once

#include <platform/config.hpp>
#include <cstdint>
#include <cstddef>

namespace Hamster
{
    class Logger
    {
    public:
        enum class LogLevel : uint8_t
        {
            LEVEL_DEBUG,
            LEVEL_INFO,
            LEVEL_WARNING,
            LEVEL_ERROR,
        };

        enum class Color : uint8_t
        {
            COLOR_DEFAULT,
            COLOR_BLACK,
            COLOR_RED,
            COLOR_GREEN,
            COLOR_YELLOW,
            COLOR_BLUE,
            COLOR_MAGENTA,
            COLOR_CYAN,
            COLOR_WHITE,
        };

        using enum LogLevel;
        using enum Color;

        class LoggerImpl
        {
        public:
            LoggerImpl(LoggerImpl &&) = delete;
            ~LoggerImpl();

            // Write messages

            LoggerImpl &operator<<(const char *);
            LoggerImpl &operator<<(char);
            LoggerImpl &operator<<(unsigned char i) { return operator<<((unsigned long long)i); }
            LoggerImpl &operator<<(signed char i) { return operator<<((long long)i); }
            LoggerImpl &operator<<(unsigned short i) { return operator<<((unsigned long long)i); }
            LoggerImpl &operator<<(short i) { return operator<<((long long)i); }
            LoggerImpl &operator<<(unsigned int i) { return operator<<((unsigned long long)i); }
            LoggerImpl &operator<<(int i) { return operator<<((long long)i); }
            LoggerImpl &operator<<(unsigned long i) { return operator<<((unsigned long long)i); }
            LoggerImpl &operator<<(long i) { return operator<<((long long)i); }
            LoggerImpl &operator<<(unsigned long long);
            LoggerImpl &operator<<(long long);
            LoggerImpl &operator<<(const void *);

            // Set color

            LoggerImpl &operator<<(Color color);

        private:
            friend class Logger;
            LoggerImpl(const char *subsystem, const char *module, bool enabled = true);
            const char *subsystem, *module;
            size_t header_len;
            bool enabled;
        };

        LoggerImpl operator()(const char *subsystem, const char *module, LogLevel level = LogLevel::LEVEL_DEBUG);
        
        void set_log_level(LogLevel level) { log_level = level; }

    private:

#define CONCAT(a, b) a##b
#define LOGLEVEL(x) CONCAT(LEVEL_, x)
        LogLevel log_level = LogLevel::LOGLEVEL(HAMSTER_DEFAULT_LOG_LEVEL);
#undef CONCAT
#undef LOGLEVEL
    };

    extern Logger logger;
} // namespace Hamster

