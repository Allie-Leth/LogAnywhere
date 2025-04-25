#pragma once

/**
 * @file LogAnywhere.h
 * @brief Single include header that exposes full logging functionality.
 *
 * This file combines handler management and logging into a single easy-to-use interface.
 */

#include "Logger.h"
#include "HandlerManager.h"
#include "HandlerEntry.h"
#include "LogLevel.h"
#include "LogMessage.h"

namespace LogAnywhere
{
    // Internal static instances
    static HandlerManager handlerManager;
    static Logger logger(&handlerManager);

    // === Handler management API ===

    /**
     * @brief Registers a new log handler with optional tag filtering and metadata.
     *
     * This function allows full configuration of a handler, including:
     * - Minimum severity level
     * - Optional context pointer
     * - Optional tag-based filter function
     * - Optional handler name (for debug/unregistration)
     * - Optional context for the filter function
     *
     * If no tag filter or name is provided, the handler will receive all messages
     * at or above the specified level.
     *
     * @param level         Minimum severity level to invoke this handler
     * @param handler       Function pointer to the handler function
     * @param context       Optional user context passed to the handler (default: nullptr)
     * @param tagFilter     Optional tag filtering function; if null, no filtering is applied (default: nullptr)
     * @param name          Optional human-readable name for the handler (default: nullptr)
     * @param filterContext Optional context passed to the tag filter function (default: nullptr)
     * @return true if registration was successful, false if handler limit exceeded
     */
    inline bool registerHandler(
        LogLevel level,
        LogHandler handler,
        void* context = nullptr,
        TagFilterFn tagFilter = nullptr,
        const char* name = nullptr,
        void* filterContext = nullptr)
    {
        return handlerManager.registerHandler(level, handler, context, tagFilter, name, filterContext);
    }

    /**
     * @brief Unregisters a handler by its unique ID.
     *
     * @param id Unique ID of the handler
     */
    inline bool unregisterHandlerByID(uint16_t id)
    {
        return handlerManager.unregisterHandlerByID(id);
    }

    /**
     * @brief Unregisters a handler by its name.
     *
     * @param name Name of the handler
     */
    inline bool unregisterHandlerByName(const char* name)
    {
        return handlerManager.unregisterHandlerByName(name);
    }

    // === Logging API ===

    /**
     * @brief Logs a preformatted message.
     *
     * @param level Severity level
     * @param tag Subsystem/component tag
     * @param message Preformatted log message
     * @param timestamp Optional timestamp
     */
    inline void log(LogLevel level, const char* tag, const char* message, uint64_t timestamp = 0)
    {
        logger.log(level, tag, message, timestamp);
    }

    /**
     * @brief Logs a formatted message using printf-style syntax.
     *
     * @param level Severity level
     * @param tag Subsystem/component tag
     * @param format printf-style format string
     * @param ... Format arguments
     */
    inline void logf(LogLevel level, const char* tag, const char* format, ...)
    {
        char buffer[256];

        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        logger.log(level, tag, buffer);
    }

    /**
     * @brief Logs a message with safety mode control (Fast or Copy).
     *
     * @param mode LogSafety::Fast or LogSafety::Copy
     * @param level Severity level
     * @param tag Subsystem/component tag
     * @param message Preformatted log message
     * @param timestamp Optional timestamp
     */
    inline void log(LogSafety mode, LogLevel level, const char* tag, const char* message, uint64_t timestamp = 0)
    {
        logger.log(mode, level, tag, message, timestamp);
    }

} // namespace LogAnywhere
