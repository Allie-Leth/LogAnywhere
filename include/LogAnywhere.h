#pragma once

/**
 * @file LogAnywhere.h
 * @brief Single-include header exposing the high-performance Tag*-based logging API.
 *
 * Version 1.1.0 removes all legacy string/tagFilter APIs in favor of exact
 * Tag* subscriptions and dispatch.  Handlers register once with a list of
 * Tag* pointers, and each log(tag) call fans out only to those subscribers.
 */

#include <cstdarg>
#include <cstdint>

#include "LogLevel.h"
#include "LogMessage.h"
#include "Tag.h"
#include "HandlerEntry.h"
#include "HandlerManager.h"
#include "Logger.h"

namespace LogAnywhere {

/** 
 * @brief Global handler registry.  
 * Used internally by all the inline functions below.
 */
static HandlerManager handlerManager;

/**
 * @brief Global logger instance wired to the above HandlerManager.
 */
static Logger logger(&handlerManager);

//=== Handler management API ===//

/**
 * @brief Register a handler for an explicit list of Tag* subscriptions.
 *
 * Each handler will only receive messages logged via those Tag* pointers.
 *
 * @param level     Minimum severity level that triggers this handler
 * @param handler   Callback to invoke for each matching LogMessage
 * @param context   User data pointer passed through to the callback
 * @param tags      Array of Tag* this handler subscribes to
 * @param tagCount  Number of entries in @p tags
 * @param name      Optional human-readable name (for diagnostics or removal)
 * @return true on success; false if the handler limit was reached
 */
inline bool registerHandler(
    LogLevel       level,
    LogHandler     handler,
    void*          context,
    const Tag*     tags[],
    size_t         tagCount,
    const char*    name = nullptr)
{
    return handlerManager.registerHandlerForTags(
        level, handler, context, tags, tagCount, name);
}

/**
 * @brief Completely remove a handler by its unique ID.
 *
 * This expensive operation prunes the handler from every Tag’s subscriber list
 * and compacts the internal registry.  Use sparingly if you need to reclaim slots.
 *
 * @param id  Unique ID of the handler to delete
 * @return true if found and removed; false otherwise
 */
inline bool deleteHandlerByID(uint16_t id)
{
    return handlerManager.deleteHandlerByID(id);
}

/**
 * @brief Completely remove a handler by its name.
 *
 * Same expensive semantics as deleteHandlerByID().
 *
 * @param name  Name of the handler to delete
 * @return true if found and removed; false otherwise
 */
inline bool deleteHandlerByName(const char* name)
{
    return handlerManager.deleteHandlerByName(name);
}

/**
 * @brief Clears all handlers and resets the internal ID counter.
 *
 * After this call, no handlers remain registered and new registrations
 * will begin again at ID = 1.
 */
inline void clearHandlers()
{
    handlerManager.clearHandlers();
}

//=== Logging API ===//

/**
 * @brief Log a preformatted message via a Tag*.
 *
 * Only handlers subscribed to tag will be invoked (and only if
 * the log level meets their threshold).
 *
 * @param level      Severity level of this message
 * @param tag        Pointer to a static Tag instance
 * @param message    Preformatted C-string
 * @param timestamp  Optional timestamp (0→use provider or sequence)
 */
inline void log(
    LogLevel     level,
    const Tag*   tag,
    const char*  message,
    uint64_t     timestamp = 0)
{
    logger.log(level, tag, message, timestamp);
}

/**
 * @brief Log a printf-style formatted message via a Tag*.
 *
 * Only handlers subscribed to tag will be invoked.
 *
 * @param level   Severity level of this message
 * @param tag     Pointer to a static Tag instance
 * @param format  printf-style format string
 * @param ...     Format arguments
 */
inline void logf(
    LogLevel     level,
    const Tag*   tag,
    const char*  format,
    ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    logger.log(level, tag, buffer);
}

} // namespace LogAnywhere
