#pragma once

/**
 * @file HandlerEntry.h
 * @brief Defines the metadata for a single log handler in the system.
 *
 * A HandlerEntry holds everything needed to route a LogMessage to the user's
 * callback function: its unique ID, severity threshold, context pointer,
 * the Tag* subscriptions it cares about, and an enabled/disabled flag.
 */

#include <cstddef>
#include <cstdint>
#include "LogLevel.h"
#include "LogMessage.h"

/// Maximum number of Tag* subscriptions per handler; can be overridden before include.
#ifndef MAX_TAG_SUBSCRIPTIONS
#define MAX_TAG_SUBSCRIPTIONS 512
#endif

namespace LogAnywhere
{
    struct Tag;
}
namespace LogAnywhere
{

    /**
     * @brief Signature of a log handler callback.
     *
     * @param msg     The fully populated log message.
     * @param context User-supplied pointer passed through registration.
     */
    using LogHandler = void (*)(const LogMessage &msg, void *context);
    /**
     * @brief Represents one registered log handler with its routing metadata.
     *
     * Each HandlerEntry is assigned a unique id by the HandlerManager
     * and may subscribe to up to MAX_TAG_SUBSCRIPTIONS Tag* channels.
     */
    struct HandlerEntry
    {
        uint16_t id;        ///< Unique identifier for this handler
        const char *name;   ///< Optional human-readable name
        LogLevel level;     ///< Minimum severity level dispatched
        LogHandler handler; ///< Function pointer for log events
        void *context;      ///< User data pointer passed to handler

        const Tag *tagList[MAX_TAG_SUBSCRIPTIONS]; ///< Fixed-size subscription list
        size_t tagCount = 0;                       ///< Actual entries in tagList[]

        bool enabled = true; ///< If false, this handler is skipped

        /**
         * @brief Default constructor. Leaves all fields zero- or default-initialized.
         */
        HandlerEntry()
            : id(0), name(nullptr), level(LogLevel::TRACE), handler(nullptr), context(nullptr),
              tagCount(0), enabled(true)
        {
            for (size_t i = 0; i < MAX_TAG_SUBSCRIPTIONS; ++i)
                tagList[i] = nullptr;
        }

        /**
         * @brief Constructs a new HandlerEntry with full metadata.
         *
         * Copies up to  MAX_TAG_SUBSCRIPTIONS pointers from tags into  tagList.
         * If count >  MAX_TAG_SUBSCRIPTIONS, the extras are silently ignored.
         *
         * @param id_        Unique handler ID assigned by HandlerManager.
         * @param name_      Optional name for diagnostics or removal.
         * @param level_     Minimum LogLevel threshold for dispatch.
         * @param fn         Callback function invoked on matching log messages.
         * @param ctx        User data pointer passed to the callback.
         * @param tags       C-style array of Tag* pointers
         * @param count      Number of elements in `tags[]` (clamped to MAX_TAG_SUBSCRIPTIONS)
         * @param enabled_   Initial enabled state (true = active).
         */
        HandlerEntry(uint16_t id_,
                     const char *name_,
                     LogLevel level_,
                     LogHandler fn,
                     void *ctx,
                     const Tag *tags[],
                     size_t count,
                     bool enabled_ = true)
            : id(id_), name(name_), level(level_), handler(fn), context(ctx), tagCount(0), enabled(enabled_)
        {
            // clamp and copy up to MAX_TAG_SUBSCRIPTIONS
            size_t toCopy = (count > MAX_TAG_SUBSCRIPTIONS)
                                ? MAX_TAG_SUBSCRIPTIONS
                                : count;
            for (size_t i = 0; i < toCopy; ++i)
            {
                tagList[i] = tags[i];
            }
            tagCount = toCopy;
        }

        /**
         * @brief Check whether this handler is currently enabled.
         *
         * @return true if the handler will be dispatched; false otherwise.
         */
        inline bool isEnabled() const noexcept { return enabled; }

        /**
         * @brief Enable or disable this handler.
         *
         * @param on Pass true to enable dispatch; false to skip this handler.
         */
        inline void setEnabled(bool on) noexcept { enabled = on; }

        /**
         * @brief Shortcut to disable this handler.
         */
        inline void disable() noexcept { enabled = false; }
    };

} // namespace LogAnywhere
