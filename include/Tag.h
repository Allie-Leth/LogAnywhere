#pragma once

/**
 * @file Tag.h
 * @brief Represents a log tag (e.g. "OTA", "CORE") and its subscribing handlers.
 */

#include <cstddef>
#include "HandlerEntry.h"

/// Each tag is ~112 bytes - Increasing this will increase memory usage.
#ifndef MAX_TAG_SUBSCRIPTIONS
#define MAX_TAG_SUBSCRIPTIONS 12
#endif

namespace LogAnywhere {

/**
 * @brief A static tag with a built-in subscriber list.
 *
 * Each Tag instance carries its own fixed array of handler pointers
 * (up to MAX_TAG_SUBSCRIPTIONS), plus a count.
 */
struct Tag
{
    const char*               name;         ///< Human-readable tag
    const HandlerEntry*       handlers[MAX_TAG_SUBSCRIPTIONS]; ///< Subscribers
    size_t                    handlerCount; ///< Number of valid entries

    /**
     * @brief Construct a Tag with a given name.
     * @param name_ The static, null-terminated C-string.
     */
    Tag(const char* name_)
        : name(name_), handlerCount(0)
    {
        // nothing else
    }
};

} // namespace LogAnywhere
