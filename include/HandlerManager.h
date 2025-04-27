#pragma once

/**
 * @file HandlerManager.h
 * @brief Manages the registration, unregistration, and storage of log handlers.
 *
 * This class is responsible for maintaining an internal list of registered
 * log handlers and their metadata. It supports severity filtering, optional
 * tag-based filtering, and optional handler naming.
 *
 * It does not perform logging — only data storage and handler lookup.
 */

#include "HandlerEntry.h"
#include "LogLevel.h"
#include <cstdint>
#include <cstring> // For strcmp

#ifndef LOGANYWHERE_MAX_HANDLERS
#define LOGANYWHERE_MAX_HANDLERS 512 // Default maximum number of handlers - can be overridden, this is purely for length of list to avoid dynamic lists.
#endif

namespace LogAnywhere
{

    class HandlerManager
    {
    public:
        /**
         * @brief Constructs an empty HandlerManager with no handlers.
         */
        HandlerManager()
            : handlerCount(0), nextHandlerId(1) {}

        /**
         * @brief Clears all registered handlers and resets the ID counter.
         *
         * After calling this, no handlers are registered and IDs start from 1 again.
         */
        void clearHandlers()
        {
            // Reset all handler entries to a default state (zero out pointers, IDs, etc.)
            std::memset(handlers, 0, sizeof(handlers));
            handlerCount = 0;
            nextHandlerId = 1;
        }

        /**
         * @brief Registers a new log handler.
         *
         * Registers a handler with optional context, tag filter, name, and
         * filter context. If the tag filter is provided, the handler will only
         * be invoked for messages where the filter returns true.
         *
         * @param level         Minimum severity level to invoke the handler
         * @param handler       Function pointer to the handler
         * @param context       Optional context passed to the handler
         * @param tagFilter     Optional tag filter function (default: nullptr)
         * @param name          Optional human-readable name (default: nullptr)
         * @param filterContext Optional context for the tag filter function (default: nullptr)
         * @return true if handler was successfully registered, false if handler limit was exceeded
         */
        bool registerHandler(
            LogLevel level,
            LogHandler handler,
            void *context = nullptr,
            TagFilterFn tagFilter = nullptr,
            const char *name = nullptr,
            void *filterContext = nullptr)
        {
            if (handlerCount >= LOGANYWHERE_MAX_HANDLERS)
                return false;

            handlers[handlerCount++] = HandlerEntry(nextHandlerId++, name, level, handler, context, tagFilter, filterContext);
            return true;
        }

        /**
         * @brief Unregisters a handler by its unique numeric ID.
         *
         * @param id The unique ID of the handler to remove
         * @return true if the handler was found and removed, false if not found
         */
        bool unregisterHandlerByID(uint16_t id)
        {
            for (size_t i = 0; i < handlerCount; ++i)
            {
                if (handlers[i].id == id)
                {
                    shiftHandlersDown(i);
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Unregisters a handler by its assigned name.
         *
         * @param name The name of the handler to remove
         * @return true if the handler was found and removed, false otherwise
         */
        bool unregisterHandlerByName(const char *name)
        {
            for (size_t i = 0; i < handlerCount; ++i)
            {
                if (handlers[i].name && strcmp(handlers[i].name, name) == 0)
                {
                    shiftHandlersDown(i);
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Lists all currently registered handlers.
         *
         * Returns a pointer to the internal handler array. The caller should use the count
         * to avoid accessing uninitialized slots. This array is owned by HandlerManager;
         * do not modify or free it.
         *
         * @param outCount [out] Populated with the number of registered handlers
         * @return Pointer to the internal handler array
         */
        const HandlerEntry *listHandlers(size_t &outCount) const
        {
            outCount = handlerCount;
            return handlers;
        }

    private:
        HandlerEntry handlers[LOGANYWHERE_MAX_HANDLERS]; ///< Internal array of registered handlers
        size_t handlerCount;                             ///< Number of active handlers
        uint16_t nextHandlerId;                          ///< Next unique handler ID to assign

        /**
         * @brief Shifts remaining handlers down to fill a removed slot.
         *
         * Used internally by unregister methods to maintain contiguous array.
         *
         * @param fromIndex Index of the removed element
         */
        void shiftHandlersDown(size_t fromIndex)
        {
            for (size_t j = fromIndex; j < handlerCount - 1; ++j)
            {
                handlers[j] = handlers[j + 1];
            }
            --handlerCount;
        }
    };

} // namespace LogAnywhere
