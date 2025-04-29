#pragma once

/**
 * @file Logger.h
 * @brief Defines the Logger class for dispatching Tag*-based log messages.
 *
 * Version 1.1.0 removes legacy string/tagFilter dispatch paths.
 */

#include "LogMessage.h"
#include "LogLevel.h"
#include "Tag.h"
#include "HandlerManager.h"
#include <cstdint>
#include <cstdarg>
#include <cstdio>

namespace LogAnywhere {

/**
 * @brief Controls how timestamp data is obtained.
 */
enum class LogSafety {
    Fast, ///< Zero-copy, not safe for async use
    Copy  ///< Deep copy of strings, safe for queuing and async
};

/**
 * @brief Central dispatcher that routes messages to Tag* subscribers.
 *
 * Logger does not own handlers; it simply asks the HandlerManager for its
 * Tag*→HandlerEntry lists and invokes each callback.
 */
class Logger {
public:
    using TimestampFn = uint64_t(*)();

    /**
     * @brief Constructs a Logger bound to a HandlerManager.
     *
     * @param manager Pointer to the HandlerManager instance
     */
    explicit Logger(const HandlerManager* manager)
      : handlerManager(manager), timestampProvider(nullptr), logSequence(1)
    {}

    /**
     * @brief Installs a custom timestamp provider function.
     *
     * If set, this function is called whenever a log timestamp of zero is
     * passed in.  Otherwise, an internal sequence counter is used.
     *
     * @param fn Function returning a uint64_t timestamp
     */
    void setTimestampProvider(TimestampFn fn) {
        timestampProvider = fn;
    }

    /**
     * @brief Logs a preformatted message via a Tag* subscription.
     *
     * Only handlers that registered for tag will be invoked, and only
     * if level >= their minimum threshold.
     *
     * @param level     Severity level of this message
     * @param tag       Tag* to dispatch against
     * @param message   Preformatted C-string message
     * @param timestamp Optional timestamp (0 ⇒ auto-generated)
     */
    void log(LogLevel level,
             const Tag* tag,
             const char* message,
             uint64_t timestamp = 0) const
    {
        if (!handlerManager) return;

        // choose timestamp
        uint64_t ts = (timestamp != 0)
          ? timestamp
          : (timestampProvider ? timestampProvider() : logSequence++);

        LogMessage msg{ level, tag->name, message, ts };

        // fast path: only subscribers of this Tag*
        for (size_t i = 0; i < tag->handlerCount; ++i) {
            const HandlerEntry* e = tag->handlers[i];
            if (!e->isEnabled())                       continue;
            if (static_cast<uint8_t>(level) < 
                static_cast<uint8_t>(e->level))       continue;
            e->handler(msg, e->context);
        }
    }

    /**
     * @brief Logs a printf-style formatted message via a Tag*.
     *
     * Formats into a fixed 256-byte buffer, then dispatches exactly as `log()`.
     *
     * @param level  Severity level of this message
     * @param tag    Tag* to dispatch against
     * @param fmt    printf-style format string
     * @param ...    Format arguments
     */
    void logf(LogLevel level,
              const Tag* tag,
              const char* fmt, ...) const
    {
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        log(level, tag, buffer);
    }

private:
    const HandlerManager* handlerManager;  ///< Where to look up handlers
    TimestampFn          timestampProvider;///< User-supplied timestamp fn
    mutable uint64_t     logSequence;      ///< Fallback auto-increment counter
};

} // namespace LogAnywhere
