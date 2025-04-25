#pragma once

/**
 * @file Logger.h
 * @brief Defines the Logger class for routing log messages to registered handlers.
 *
 * Logger formats messages, applies timestamps, and dispatches them to handlers managed elsewhere.
 * Logger does not own or manage handlers; it only routes to them.
 */

#include "LogMessage.h"
#include "LogLevel.h"
#include "HandlerEntry.h"
#include "HandlerManager.h"
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstring> // For strdup
#include <cstdlib> // For free

namespace LogAnywhere
{
    /**
     * @brief Safety mode for controlling how log data is handled.
     */
    enum class LogSafety
    {
        Fast, ///< Zero-copy, not safe for async use
        Copy  ///< Deep copy of strings, safe for queuing and async
    };

    /**
     * @brief Central log dispatcher.
     *
     * Logger receives log messages and routes them to all handlers
     * registered inside the provided HandlerManager.
     */
    class Logger
    {
    public:
        using TimestampFn = uint64_t (*)();

        /**
         * @brief Constructs a Logger with an associated HandlerManager.
         *
         * @param manager Pointer to the HandlerManager to use for dispatching logs.
         */
        explicit Logger(const HandlerManager* manager)
            : handlerManager(manager), timestampProvider(nullptr), logSequence(1) {}

        /**
         * @brief Sets a custom timestamp provider function.
         *
         * @param fn Pointer to a timestamp function returning uint64_t
         */
        void setTimestampProvider(TimestampFn fn)
        {
            timestampProvider = fn;
        }

        /**
         * @brief Logs a preformatted message.
         *
         * @param level     Severity level
         * @param tag       Subsystem/component name
         * @param message   Formatted log message
         * @param timestamp Optional timestamp; if 0, uses timestamp provider or sequence counter
         */
        void log(LogLevel level, const char* tag, const char* message, uint64_t timestamp = 0) const
        {
            if (!handlerManager)
                return;

            uint64_t ts = (timestamp != 0)
                        ? timestamp
                        : (timestampProvider ? timestampProvider() : logSequence++);

            LogMessage msg{level, tag, message, ts};

            size_t count = 0;
            const HandlerEntry* handlers = handlerManager->listHandlers(count);

            for (size_t i = 0; i < count; ++i)
            {
                const HandlerEntry& entry = handlers[i];

                // Severity filter
                if (static_cast<uint8_t>(level) < static_cast<uint8_t>(entry.level))
                    continue;

                // Tag filter if provided
                if (entry.tagFilter && !entry.tagFilter(tag, entry.filterContext))
                    continue;

                // Dispatch
                entry.handler(msg, entry.context);
            }
        }

        /**
         * @brief Logs a formatted message using printf-style syntax.
         */
        void logf(LogLevel level, const char* tag, const char* format, ...) const
        {
            char buffer[256];

            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);

            log(level, tag, buffer);
        }

        /**
         * @brief Logs a message safely for asynchronous handlers.
         *
         * Deep copies strings for memory safety.
         */
        void log(LogSafety mode, LogLevel level, const char* tag, const char* message, uint64_t timestamp = 0) const
        {
            if (mode == LogSafety::Fast)
            {
                log(level, tag, message, timestamp);
                return;
            }

            char* tagCopy = strdup(tag);
            char* msgCopy = strdup(message);

            log(level, tagCopy, msgCopy, timestamp);

            free(tagCopy);
            free(msgCopy);
        }

    private:
        const HandlerManager* handlerManager; ///< Pointer to external HandlerManager
        TimestampFn timestampProvider;        ///< Optional timestamp provider
        mutable uint64_t logSequence;          ///< Fallback sequence counter (mutable to allow const methods)
    };

} // namespace LogAnywhere
