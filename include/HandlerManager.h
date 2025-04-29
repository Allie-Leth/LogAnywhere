#pragma once

/**
 * @file HandlerManager.h
 * @brief Manages registration and removal of log handlers by Tag* subscriptions.
 *
 * Provides:
 *  - Fast, exact‐match registration via Tag* pointers
 *  - Full removal (pruning from Tag subscriber lists and compacting registry)
 *  - Clearing all handlers and resetting IDs
 *  - Listing current handlers
 *
 * Does not perform dispatch; that is done by Logger.
 */

#include "HandlerEntry.h"
#include "Tag.h"
#include "LogLevel.h"
#include <cstdint>
#include <cstddef>
#include <cstring> // for std::strcmp

// Increasing the max handlers will increase memory usage - 
// each one has a list of all tags, so increasing both gets dangerous
#ifndef LOGANYWHERE_MAX_HANDLERS
#define LOGANYWHERE_MAX_HANDLERS 6
#endif

namespace LogAnywhere
{

    /**
     * @brief Stores and manages all registered HandlerEntry instances.
     */
    class HandlerManager
    {
    public:
        /**
         * @brief Constructs an empty HandlerManager.
         *
         * IDs will start at 1 for the first registered handler.
         */
        HandlerManager()
            : handlerCount(0),
              nextHandlerId(1)
        {
        }

        /**
         * @brief Clears all registered handlers and resets the ID counter.
         *
         * After this call, no handlers remain registered. New registrations
         * will begin again at ID = 1.
         */
        void clearHandlers()
        {
            // NOTE: Tag subscriber lists are not automatically cleared.
            std::memset(handlers, 0, sizeof(handlers));
            handlerCount = 0;
            nextHandlerId = 1;
        }

        /**
         * @brief Registers a handler for an explicit list of Tag* subscriptions.
         *
         * When a log is emitted via one of the Tag* in tagList, this handler
         * will be invoked (provided its severity threshold is met).
         *
         * @param level     Minimum log level to invoke this handler.
         * @param fn        Callback function pointer.
         * @param ctx       User-supplied context passed to the callback.
         * @param tagList   Array of Tag* that this handler subscribes to.
         * @param tagCount  Number of elements in  tagList.
         * @param name      Optional handler name for diagnostics or removal.
         * @return true if registration succeeded, false if capacity is exceeded.
         */
        bool registerHandlerForTags(
            LogLevel level,
            LogHandler fn,
            void *ctx,
            const Tag **tagList,
            size_t tagCount,
            const char *name = nullptr)
        {
            if (handlerCount >= LOGANYWHERE_MAX_HANDLERS)
                return false;

            // Create the entry
            handlers[handlerCount] = HandlerEntry(
                nextHandlerId++, // id
                name,            // name
                level,           // severity
                fn,              // handler fn
                ctx,             // user context
                tagList,         // Tag* list
                tagCount,        // number of tags
                true             // enabled
            );

            // Grab its pointer for subscription
            HandlerEntry *entry = &handlers[handlerCount++];

            // Subscribe into each Tag's handler array
            for (size_t i = 0; i < tagCount; ++i)
            {
                Tag *t = const_cast<Tag *>(tagList[i]);
                if (t->handlerCount < MAX_TAG_SUBSCRIPTIONS)
                {
                    t->handlers[t->handlerCount++] = entry;
                }
            }

            return true;
        }

        /**
         * @brief Deletes (fully removes) a handler by its unique ID.
         *
         * This expensive operation:
         *  1) Prunes the handler from every Tag::handlers[] it subscribed to.
         *  2) Removes it from the internal registry (compact array).
         *
         * @param id  Unique ID of the handler to remove.
         * @return true if found and deleted; false otherwise.
         */
        bool deleteHandlerByID(uint16_t id)
        {
            for (size_t i = 0; i < handlerCount; ++i)
            {
                if (handlers[i].id == id)
                {
                    // Prune from each Tag's subscriber list
                    HandlerEntry &e = handlers[i];
                    for (size_t t = 0; t < e.tagCount; ++t)
                    {
                        Tag *tag = const_cast<Tag *>(e.tagList[t]);
                        size_t w = 0;
                        for (size_t r = 0; r < tag->handlerCount; ++r)
                        {
                            if (tag->handlers[r]->id != id)
                                tag->handlers[w++] = tag->handlers[r];
                        }
                        tag->handlerCount = w;
                    }
                    // Compact the registry
                    shiftHandlersDown(i);
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Deletes (fully removes) a handler by its name.
         *
         * Same expensive semantics as deleteHandlerByID().
         *
         * @param name  Name of the handler to remove.
         * @return true if found and deleted; false otherwise.
         */
        bool deleteHandlerByName(const char *name)
        {
            for (size_t i = 0; i < handlerCount; ++i)
            {
                if (handlers[i].name && std::strcmp(handlers[i].name, name) == 0)
                {
                    // Prune from Tags
                    HandlerEntry &e = handlers[i];
                    for (size_t t = 0; t < e.tagCount; ++t)
                    {
                        Tag *tag = const_cast<Tag *>(e.tagList[t]);
                        size_t w = 0;
                        for (size_t r = 0; r < tag->handlerCount; ++r)
                        {
                            if (!(tag->handlers[r]->name &&
                                  std::strcmp(tag->handlers[r]->name, name) == 0))
                            {
                                tag->handlers[w++] = tag->handlers[r];
                            }
                        }
                        tag->handlerCount = w;
                    }
                    // Compact registry
                    shiftHandlersDown(i);
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Lists all registered handlers (enabled or disabled).
         *
         * @param outCount  Populated with the number of handlers in the array.
         * @return Pointer to the internal HandlerEntry array.
         */
        const HandlerEntry *listHandlers(size_t &outCount) const
        {
            outCount = handlerCount;
            return handlers;
        }

    private:
        HandlerEntry handlers[LOGANYWHERE_MAX_HANDLERS]; ///< Storage of all handlers
        size_t handlerCount;                             ///< Number of active entries
        uint16_t nextHandlerId;                          ///< Next ID to assign

        /**
         * @brief Removes the entry at @p index from the registry and compacts.
         *
         * @param index  Index of handler to remove.
         */
        void shiftHandlersDown(size_t index)
        {
            for (size_t j = index; j + 1 < handlerCount; ++j)
                handlers[j] = handlers[j + 1];
            --handlerCount;
        }
    };

} // namespace LogAnywhere
