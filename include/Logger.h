#pragma once

/**
 * @file Logger.h
 * @brief Defines the Logger class for routing log messages to user-defined handlers.
 *
 * This header contains the implementation of LogAnywhere's core Logger class.
 * It supports log filtering by severity level and optionally by message tag.
 */

#include "LogMessage.h"
#include "LogLevel.h"
#include "HandlerEntry.h"
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstring> // For strdup
#include <cstdlib> // For free

namespace LogAnywhere
{
    uint64_t sequence = 0; // Strictly increasing log order -- default logging behavior is no timestamp provided

    /**
     * @brief A user-defined log handler function.
     *
     * This function receives a `LogMessage` and a user-defined context pointer.
     * It is called whenever a log message matches the handler's registered criteria.
     */
    using LogHandler = void (*)(const LogMessage &, void *context);

    /**
     * @brief Optional tag filter function.
     *
     * If provided, this function will be invoked before calling the associated log handler.
     * The filter can inspect the `tag` and return true to allow the message or false to skip it.
     */
    using TagFilterFn = bool (*)(const char *tag, void *context);

    /**
     * @brief Safety mode for controlling how log data is handled.
     *
     * Use `Fast` for stack-only, zero-copy logging.
     * Use `Copy` when logging to asynchronous handlers to avoid dangling pointers.
     */
    enum class LogSafety
    {
        Fast, ///< Zero-copy, not safe for async use
        Copy  ///< Deep copy of strings, safe for queuing and async
    };

    /**
     * @brief Central log dispatcher that manages log routing and handler filtering.
     */

    class Logger
    {
    public:
        static constexpr size_t MaxHandlers = 8; ///< Maximum number of registered handlers
        HandlerEntry handlers[MaxHandlers];        ///< Array of registered handlers
        uint64_t logSequence = 1;                ///< Sequence number for log messages if no timestamp is provided
        uint16_t nextHandlerId = 1;              ///< Unique ID for the next handler
        /**
         * @brief Optional user-supplied timestamp function.
         *
         * This function will be used by the logger to generate timestamps for
         * log messages if the caller does not explicitly provide one.
         *
         * The function must return a 64-bit unsigned integer representing
         * time since boot, wall-clock time, or any other time base the user prefers.
         *
         * If not set, the logger will fall back to its internal default provider,
         * typically a monotonic time source (e.g., `esp_timer_get_time()` or `steady_clock`).
         */
        using TimestampFn = uint64_t (*)();

        /**
         * @brief Sets a custom timestamp provider function.
         *
         * This allows the user to override the logger’s default time source.
         * The function you provide will be used to generate timestamps for all
         * log messages where a timestamp is not manually supplied.
         *
         * Example use cases:
         * - Replace time since boot with a real-time clock (RTC)
         * - Sync with NTP and provide epoch-based time
         * - Use high-resolution timers
         *
         * @param fn Pointer to a function returning a uint64_t timestamp
         */
        void setTimestampProvider(TimestampFn fn)
        {
            timestampProvider = fn;
        }

        /**
         * @brief Constructs a Logger with no registered handlers.
         */
        Logger() : handlerCount(0) {}

        /**
         * @brief Registers a handler with no tag filtering.
         *
         * The handler will receive all messages at or above the specified level.
         *
         * @param level     Minimum severity to trigger this handler
         * @param handler   Function pointer to the log output function
         * @param context   Optional user data passed to the handler
         * @return true if the handler was successfully registered
         */
        bool registerHandler(LogLevel level, LogHandler handler, void *context = nullptr, const char *name = nullptr)
        {
            if (handlerCount >= MaxHandlers) return false;


            handlers[handlerCount++] = HandlerEntry(nextHandlerId++, name, level, handler, context);
            return true;

            return true;
        }

        /**
         * @brief Registers a handler with a tag-based filter.
         *
         * The handler will only receive messages if the tag filter returns true.
         *
         * @param level          Minimum severity level to trigger this handler
         * @param handler        Function pointer to the log output function
         * @param context        Optional user context for the handler
         * @param tagFilter      Function pointer for filtering by tag
         * @param filterContext  Optional context passed to the filter function
         * @return true if the handler was successfully registered
         */
        inline bool registerHandlerFiltered(
            LogLevel level,
            LogHandler handler,
            void* context,
            TagFilterFn tagFilter,
            const char* name = nullptr,
            void* filterContext = nullptr
        ) {
            if (handlerCount >= MaxHandlers)
                return false;

            handlers[handlerCount++] = {
                static_cast<uint16_t>(handlerCount),
                name,
                level,
                handler,
                context,
                tagFilter,
                filterContext
            };
            return true;
        }

        /**
         * @brief Logs a preformatted message to all matching handlers.
         *
         * Each handler is evaluated for severity level and optional tag filtering.
         *
         * @param level     Severity level of the message
         * @param tag       Descriptive tag for the source of the message
         * @param message   Fully formatted log message string
         * @param timestamp Optional timestamp (UTC or monotonic); default is 0
         */
        void log(LogLevel level, const char *tag, const char *message, uint64_t timestamp = 0)
        {
            uint64_t ts = (timestamp != 0) ? timestamp : (timestampProvider ? timestampProvider() : 0, logSequence++);
            LogMessage msg{level, tag, message, ts};

            for (size_t i = 0; i < handlerCount; ++i)
            {
                const auto &entry = handlers[i];

                // Check severity level threshold
                if (static_cast<uint8_t>(level) >= static_cast<uint8_t>(entry.level))
                {
                    // Check optional tag filter
                    if (entry.tagFilter &&
                        !entry.tagFilter(tag, entry.filterContext))
                    {
                        continue;
                    }

                    // Invoke handler
                    entry.handler(msg, entry.context);
                }
            }
        }

        /**
         * @brief Logs a formatted message using printf-style formatting.
         *
         * Uses a fixed-size internal buffer for formatting; message is then routed
         * using the standard `log()` method.
         *
         * @param level   Severity level of the message
         * @param tag     Tag identifying the subsystem/component
         * @param format  printf-style format string
         * @param ...     Arguments to format
         */
        void logf(LogLevel level, const char *tag, const char *format, ...)
        {
            char buffer[256]; // Static buffer — no dynamic allocation

            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);

            log(level, tag, buffer);
        }

        /**
         * @brief Logs a message in a memory-safe way for asynchronous handlers.
         *
         * Copies the tag and message strings to heap-allocated memory, making the message
         * safe to queue or pass to a background thread. Caller is responsible for memory cleanup
         * or using handlers that manage lifecycle.
         *
         * @param mode     LogSafety::Fast or LogSafety::Copy
         * @param level    Severity level
         * @param tag      Subsystem identifier
         * @param message  Preformatted message string
         * @param timestamp Optional timestamp
         */
        void log(LogSafety mode, LogLevel level, const char *tag, const char *message, uint64_t timestamp = 0)
        {
            if (mode == LogSafety::Fast)
            {
                log(level, tag, message, timestamp);
                return;
            }

            // Deep-copy tag and message
            char *tagCopy = strdup(tag);
            char *msgCopy = strdup(message);

            LogMessage msg{level, tagCopy, msgCopy, timestamp};
            log(msg.level, msg.tag, msg.message, msg.timestamp);

            // Cleanup — assumes handler doesn’t retain pointers
            free(tagCopy);
            free(msgCopy);
        }

    private:
        HandlerEntry handlers[MaxHandlers]; ///< Array of registered handlers
        size_t handlerCount;                ///< Number of handlers currently registered
        TimestampFn timestampProvider = nullptr;
    };

} // namespace LogAnywhere
