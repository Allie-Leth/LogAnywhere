#pragma once
/**
 * @file Logger.h
 * @brief Defines the Logger class for dispatching Tag*-based log messages.
 *
 * Version 1.2.0: Refactored into single‐purpose private helpers for clarity
 * and testability.
 */

#include "LogMessage.h"
#include "LogLevel.h"
#include "Tag.h"
#include "HandlerManager.h"
#include <cstdint>
#include <cstdarg>
#include <cstdio>

namespace LogAnywhere
{

    /**
     * @brief Controls how timestamp data is obtained.
     */
    enum class LogSafety
    {
        Fast, ///< Zero-copy, not safe for async use
        Copy  ///< Deep copy of strings, safe for queuing and async use
    };

    /**
     * @brief Central dispatcher that routes messages to Tag* subscribers.
     *
     * Logger does not own handlers; it simply asks the HandlerManager for its
     * Tag*→HandlerEntry lists and invokes each callback.
     */
    class Logger
    {
    public:
        /// Function signature for a custom timestamp provider
        using TimestampFn = uint64_t (*)();

        /**
         * @brief Constructs a Logger bound to a HandlerManager.
         *
         * @param manager Pointer to the HandlerManager instance.
         */
        explicit Logger(const HandlerManager *manager)
            : handlerManager(manager), timestampProvider(nullptr), logSequence(1)
        {
        }

        /**
         * @brief Installs a custom timestamp provider function.
         *
         * If set, this function is called whenever a log timestamp of zero is
         * passed in. Otherwise, an internal sequence counter is used.
         *
         * @param fn Function returning a uint64_t timestamp.
         */
        void setTimestampProvider(TimestampFn fn)
        {
            timestampProvider = fn;
        }

        /**
         * @brief Logs a preformatted message via a Tag* subscription.
         *
         * Only handlers that registered for @p tag will be invoked, and only
         * if @p level is ≥ their minimum threshold.
         *
         * @param level     Severity level of this message.
         * @param tag       Tag* to dispatch against.
         * @param message   Preformatted C-string message.
         * @param timestamp Optional timestamp (0 ⇒ auto-generated).
         */
        void log(LogLevel level,
                 const Tag *tag,
                 const char *message,
                 uint64_t timestamp = 0) const;

        /**
         * @brief Logs a printf-style formatted message via a Tag*.
         *
         * Formats into a fixed 256-byte stack buffer, then dispatches exactly as
         * `log()`.
         *
         * @param level Severity level of this message.
         * @param tag   Tag* to dispatch against.
         * @param fmt   printf-style format string.
         * @param ...   Format arguments.
         */
        void logf(LogLevel level,
                  const Tag *tag,
                  const char *fmt, ...) const;

    private:
        const HandlerManager *handlerManager; ///< Where to look up handlers
        TimestampFn timestampProvider;        ///< User-supplied timestamp fn
        mutable uint64_t logSequence;         ///< Fallback auto-increment counter

        /**
         * @brief Chooses the correct timestamp for a log entry.
         *
         * If @p explicitTs is non-zero, returns it; else if a timestampProvider is
         * installed, calls it; otherwise increments and returns logSequence.
         *
         * @param explicitTs User-provided timestamp (0 ⇒ none).
         * @return The final timestamp to use for the log message.
         */
        uint64_t chooseTimestamp(uint64_t explicitTs) const;

        /**
         * @brief Constructs the LogMessage struct from core fields.
         *
         * @param level       Severity level of this message.
         * @param tag         Tag* of this message.
         * @param message     C-string message content.
         * @param explicitTs  User-provided timestamp (0 ⇒ auto-generated).
         * @return A fully populated LogMessage instance.
         */
        LogMessage makeMessage(LogLevel level,
                               const Tag *tag,
                               const char *message,
                               uint64_t explicitTs) const;

        /**
         * @brief Dispatches a LogMessage to all eligible handlers for a tag.
         *
         * Invokes each subscriber whose level threshold is met and is enabled.
         *
         * @param msg The log message to dispatch.
         * @param tag The Tag* whose subscriber list to walk.
         */
        void dispatchToHandlers(const LogMessage &msg,
                                const Tag *tag) const;
    };
    /**
     * @brief Chooses the correct timestamp for a log entry.
     *
     * If @p explicitTs is non-zero, returns it; else if a custom
     * @c timestampProvider is set, calls that; otherwise returns
     * the next value of the internal auto-incremented counter.
     *
     * @param explicitTs User-supplied timestamp (0 ⇒ none)
     * @return Final timestamp to stamp the log with
     */
    inline uint64_t Logger::chooseTimestamp(uint64_t explicitTs) const
    {
        if (explicitTs != 0)
            return explicitTs;
        if (timestampProvider)
            return timestampProvider();
        return logSequence++;
    }

    /**
     * @brief Builds a LogMessage object from primitive inputs.
     *
     * Populates a @c LogMessage with level, tag name, message content,
     * and a timestamp chosen via @c chooseTimestamp().
     *
     * @param level      Severity level for this message
     * @param tag        Pointer to the Tag this message belongs to
     * @param message    C-string payload
     * @param explicitTs User-supplied timestamp (0 ⇒ auto-generated)
     * @return A fully populated LogMessage
     */
    inline LogMessage Logger::makeMessage(LogLevel level,
                                          const Tag *tag,
                                          const char *message,
                                          uint64_t explicitTs) const
    {
        return LogMessage(level,
                          tag->name,
                          message,
                          chooseTimestamp(explicitTs));
    }

    /**
     * @brief Sends a LogMessage to every enabled handler subscribed to a Tag.
     *
     * Iterates through the Tag’s subscriber list and invokes each handler
     * whose severity threshold is met.
     *
     * @param msg The LogMessage to dispatch
     * @param tag The Tag whose handlers will receive @p msg
     */
    inline void Logger::dispatchToHandlers(const LogMessage &msg,
                                           const Tag *tag) const
    {
        for (size_t i = 0; i < tag->handlerCount; ++i)
        {
            const HandlerEntry *e = tag->handlers[i];
            if (!e->isEnabled())
                continue;
            if (static_cast<uint8_t>(msg.level) <
                static_cast<uint8_t>(e->level))
                continue;
            e->handler(msg, e->context);
        }
    }

    /**
     * @brief Logs a preformatted message via a Tag* subscription.
     *
     * Forwards the message through the timestamp helper and then dispatches
     * to all handlers registered under @p tag.
     *
     * @param level     Severity level of this message
     * @param tag       Tag* to dispatch against
     * @param message   Preformatted C-string message
     * @param timestamp Optional timestamp (0 ⇒ auto-generated)
     */
    inline void Logger::log(LogLevel level,
                            const Tag *tag,
                            const char *message,
                            uint64_t timestamp /*=0*/) const
    {
        if (!handlerManager)
            return;
        auto msg = makeMessage(level, tag, message, timestamp);
        dispatchToHandlers(msg, tag);
    }

    /**
     * @brief Logs a printf-style formatted message via a Tag*.
     *
     * Formats arguments into a fixed 256-byte buffer, then calls the
     * standard @c log() path.
     *
     * @param level Severity level of this message
     * @param tag   Tag* to dispatch against
     * @param fmt   printf-style format string
     * @param ...   Format arguments
     */
    inline void Logger::logf(LogLevel level,
                             const Tag *tag,
                             const char *fmt, ...) const
    {
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        log(level, tag, buffer);
    }
}