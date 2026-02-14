
#include <logger/logger.hpp>
#include <platform/platform.hpp>
#include <platform/config.hpp>
#include <cstring>
#include <cstdio>
#include <cstdlib>

namespace Hamster
{
    Logger::LoggerImpl::LoggerImpl(const char *subsystem, const char *module, bool enabled)
        : subsystem(subsystem), module(module), header_len(1 /*[*/ + strlen(subsystem) + 2 /*::*/ + strlen(module) + 2 /*] */),
          enabled(enabled)
    {
        if (enabled)
        {
            *this << Color::COLOR_DEFAULT;

            // Timestamp
            uint64_t now = _get_sys_time();
            uint64_t seconds = now / 1000;
            uint64_t milliseconds = now % 1000;

            size_t timestap_len = snprintf(nullptr, 0, "[%.5llu.%03llus] ", (unsigned long long)seconds, (unsigned long long)milliseconds);
            header_len += timestap_len;
            char *timestamp_buf = (char *)alloca(timestap_len + 1);
            snprintf(timestamp_buf, timestap_len + 1, "[%.5llu.%03llus] ", (unsigned long long)seconds, (unsigned long long)milliseconds);

            _trace("%s[%s::%s] ", timestamp_buf, subsystem, module);
        }
    }

    Logger::LoggerImpl::~LoggerImpl()
    {
        if (enabled)
            _trace("\r\n");
    }

    Logger::LoggerImpl &Logger::LoggerImpl::operator<<(const char *msg)
    {
        if (!enabled)
            return *this;
        for (const char *it = msg; *it; it++)
            *this << *it;
        return *this;
    }

    Logger::LoggerImpl &Logger::LoggerImpl::operator<<(char c)
    {
        if (!enabled)
            return *this;
        _trace("%c", c);
        if (c == '\r' || c == '\n')
            // pad for header
            _trace("%*s", (int)header_len, "");
        return *this;
    }

    Logger::LoggerImpl &Logger::LoggerImpl::operator<<(long long num)
    {
        if (!enabled)
            return *this;
        _trace("%lld", num);
        return *this;
    }

    Logger::LoggerImpl &Logger::LoggerImpl::operator<<(unsigned long long num)
    {
        if (!enabled)
            return *this;
        _trace("%llu", num);
        return *this;
    }

    Logger::LoggerImpl &Logger::LoggerImpl::operator<<(const void *ptr)
    {
        if (!enabled)
            return *this;
        _trace("%p", ptr);
        return *this;
    }

    Logger::LoggerImpl &Logger::LoggerImpl::operator<<(Color color)
    {
        if (!enabled)
            return *this;
#if !defined(HAMSTER_LOGGER_NO_COLORS) || HAMSTER_LOGGER_NO_COLORS == 0
        _trace("\033[");
        switch (color)
        {
        case Color::COLOR_BLACK:
            _trace("30m");
            break;
        case Color::COLOR_RED:
            _trace("31m");
            break;
        case Color::COLOR_GREEN:
            _trace("32m");
            break;
        case Color::COLOR_YELLOW:
            _trace("33m");
            break;
        case Color::COLOR_BLUE:
            _trace("34m");
            break;
        case Color::COLOR_MAGENTA:
            _trace("35m");
            break;
        case Color::COLOR_CYAN:
            _trace("36m");
            break;
        case Color::COLOR_WHITE:
            _trace("37m");
            break;
        default:
            _trace("0m");
            break;
        }
#endif
        return *this;
    }

    Logger::LoggerImpl Logger::operator()(const char *subsystem, const char *module, LogLevel level)
    {
        bool enabled = (uint8_t)level >= (uint8_t)log_level;
        return LoggerImpl(subsystem, module, enabled);
    }
} // namespace Hamster

