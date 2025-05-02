#pragma once

/**
 * @file HandlerManager.h
 * @brief Manages registration, lookup, and removal of log handlers by Tag* subscriptions.
 *
 * HandlerManager stores up to LOGANYWHERE_MAX_HANDLERS HandlerEntry instances,
 * each of which may subscribe to a fixed list of Tag* pointers.  It supports:
 *  - Fast, exact‐match registration via Tag* pointers
 *  - Full removal (pruning from Tag subscriber lists and compacting registry)
 *  - Clearing all handlers and resetting IDs
 *  - Listing current handlers
 *
 * Dispatching is performed by Logger, not by this class.
 */

#include "HandlerEntry.h"
#include "Tag.h"
#include "LogLevel.h"
#include <cstdint>
#include <cstddef>
#include <cstring>

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
         *
         * @note Tag subscriber lists are _not_ automatically cleared.
         */
        void clearHandlers()
        {
            // Use a temp copy so we can unsubscribe safely before the memory is wiped
            for (size_t i = 0; i < handlerCount; ++i)
            {
                HandlerEntry *e = &handlers[i];
                // Make a local copy of the tag list to safely unsubscribe
                const Tag *tags[MAX_TAG_SUBSCRIPTIONS];
                std::memcpy(tags, e->tagList, sizeof(tags));
                size_t tagCount = e->tagCount;

                for (size_t t = 0; t < tagCount; ++t)
                {
                    Tag *tag = const_cast<Tag *>(tags[t]);
                    size_t wr = 0;
                    for (size_t rd = 0; rd < tag->handlerCount; ++rd)
                    {
                        if (tag->handlers[rd] != e)
                            tag->handlers[wr++] = tag->handlers[rd];
                    }
                    tag->handlerCount = wr;
                }
            }

            // Now safe to wipe handler state
            for (size_t i = 0; i < MAX_HANDLERS; ++i)
            {
                handlers[i] = HandlerEntry{}; // value‑init each slot
            }
            handlerCount = 0;
            nextHandlerId = 1;
        }
        /**
         * @brief Lists all registered handlers.
         *
         * @param outCount Populated with the number of handlers in the array.
         * @return Pointer to the internal HandlerEntry array.
         */
        const HandlerEntry *listHandlers(size_t &outCount) const
        {
            outCount = handlerCount;
            return handlers;
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
         * @param tagCount  Number of elements in tagList.
         * @param name      Optional handler name for diagnostics or removal.
         * @return true if registration succeeded, false if capacity is exceeded.
         */
        bool registerHandlerForTags(LogLevel level,
                                    LogHandler fn,
                                    void *ctx,
                                    const Tag *tagList[],
                                    size_t tagCount,
                                    const char *name = nullptr);

        /**
         * @brief Deletes a handler by its unique ID.
         *
         * This expensive operation:
         *  1) Prunes the handler from every Tag::handlers[] it subscribed to.
         *  2) Removes it from the internal registry.
         *
         * @param id Unique ID of the handler to remove.
         * @return true if found and deleted; false otherwise.
         */
        bool deleteHandlerByID(uint16_t id);

        /**
         * @brief Deletes a handler by its name.
         *
         * Same expensive semantics as deleteHandlerByID().
         *
         * @param name Name of the handler to remove.
         * @return true if found and deleted; false otherwise.
         */
        bool deleteHandlerByName(const char *name);

    private:
        static constexpr size_t MAX_HANDLERS = LOGANYWHERE_MAX_HANDLERS; ///< Capacity of handlers[]
        HandlerEntry handlers[MAX_HANDLERS];                             ///< Storage of all handlers
        size_t handlerCount;                                             ///< Number of active entries
        uint16_t nextHandlerId;                                          ///< Next ID to assign

        // --------------------------------------------------------------------
        // Single‐purpose helper methods
        // --------------------------------------------------------------------

        /**
         * @brief Checks if there's capacity for another handler.
         * @return true if handlerCount < MAX_HANDLERS.
         */
        bool hasCapacity() const;

        /**
         * @brief Constructs a new HandlerEntry in handlers[] and increments count.
         * @param level     Severity threshold.
         * @param fn        Callback function.
         * @param ctx       User context.
         * @param tagList   Tags to subscribe to.
         * @param tagCount  Number of tags.
         * @param name      Optional name.
         * @return Reference to the newly created HandlerEntry.
         */
        HandlerEntry &createEntry(LogLevel level,
                                  LogHandler fn,
                                  void *ctx,
                                  const Tag *tagList[],
                                  size_t tagCount,
                                  const char *name);

        /**
         * @brief Subscribes a HandlerEntry to each Tag in tagList.
         * @param entry     The HandlerEntry to subscribe.
         * @param tagList   Array of Tag* pointers.
         * @param tagCount  Number of tags to subscribe to.
         */
        void subscribeEntryToTags(HandlerEntry *entry,
                                  const Tag *tagList[],
                                  size_t tagCount);

        /**
         * @brief Finds a handler entry by its ID.
         * @param id Unique handler ID.
         * @return Pointer to the HandlerEntry or nullptr if not found.
         */
        HandlerEntry *findEntryByID(uint16_t id);

        /**
         * @brief Finds a handler entry by its name.
         * @param name Handler name to search.
         * @return Pointer to the HandlerEntry or nullptr if not found.
         */
        HandlerEntry *findEntryByName(const char *name);

        /**
         * @brief Unsubscribes a HandlerEntry from all Tags it was on.
         * @param entry The HandlerEntry to remove from tags.
         */
        void unsubscribeEntryFromTags(HandlerEntry *entry);

        /**
         * @brief Compacts out the slot at index in handlers[].
         * After this, handlerCount is decremented.
         * @param index Index of the slot to remove.
         */
        void compactHandlerArray(size_t index);
    };

    // ──────────────────────────────────────────────────────────────────────────────
    // Inline definitions of the helper and API methods
    // ──────────────────────────────────────────────────────────────────────────────

    /**
     * @brief Checks if there is room for another handler.
     *
     * @return true if the current number of handlers is less than the maximum allowed.
     */
    inline bool HandlerManager::hasCapacity() const
    {
        return handlerCount < MAX_HANDLERS;
    }

    /**
     * @brief Constructs a new HandlerEntry in the internal array.
     *
     * Creates a HandlerEntry with the given parameters at the next free slot,
     * assigns it a unique ID, and increments the handler count.
     *
     * @param level     Minimum log level threshold for this handler.
     * @param fn        Callback function pointer to invoke on log dispatch.
     * @param ctx       User-supplied context passed to the callback.
     * @param tagList   Array of Tag* pointers this handler should subscribe to.
     * @param tagCount  Number of tags in @p tagList.
     * @param name      Optional human-readable name for diagnostics or removal.
     * @return Reference to the newly created HandlerEntry.
     */
    inline HandlerEntry &HandlerManager::createEntry(LogLevel level,
                                                     LogHandler fn,
                                                     void *ctx,
                                                     const Tag *tagList[],
                                                     size_t tagCount,
                                                     const char *name)
    {
        HandlerEntry &e = handlers[handlerCount];
        e = HandlerEntry(nextHandlerId++, name, level, fn, ctx, tagList, tagCount, true);
        return handlers[handlerCount++];
    }

    /**
     * @brief Subscribes a HandlerEntry to each Tag in the provided list.
     *
     * For each tag in @p tagList, if there is capacity, appends the
     * @p entry pointer into that Tag's subscriber array.
     *
     * @param entry     Pointer to the HandlerEntry to subscribe.
     * @param tagList   Array of Tag* pointers to subscribe to.
     * @param tagCount  Number of tags in @p tagList.
     */
    inline void HandlerManager::subscribeEntryToTags(HandlerEntry *entry,
                                                     const Tag *tagList[],
                                                     size_t tagCount)
    {
        for (size_t i = 0; i < tagCount; ++i)
        {
            Tag *t = const_cast<Tag *>(tagList[i]);
            if (t->handlerCount < MAX_TAG_SUBSCRIPTIONS)
            {
                t->handlers[t->handlerCount++] = entry;
            }
        }
    }

    /**
     * @brief Finds a registered handler by its unique ID.
     *
     * Iterates through the internal handler array and returns a pointer
     * to the entry whose ID matches @p id.
     *
     * @param id Unique identifier of the handler.
     * @return Pointer to the HandlerEntry if found, nullptr otherwise.
     */
    inline HandlerEntry *HandlerManager::findEntryByID(uint16_t id)
    {
        for (size_t i = 0; i < handlerCount; ++i)
        {
            if (handlers[i].id == id)
                return &handlers[i];
        }
        return nullptr;
    }

    /**
     * @brief Finds a registered handler by its assigned name.
     *
     * Iterates through the internal handler array and returns a pointer
     * to the entry whose name matches @p name.
     *
     * @param name Null-terminated string name of the handler.
     * @return Pointer to the HandlerEntry if found, nullptr otherwise.
     */
    inline HandlerEntry *HandlerManager::findEntryByName(const char *name)
    {
        for (size_t i = 0; i < handlerCount; ++i)
        {
            if (handlers[i].name && std::strcmp(handlers[i].name, name) == 0)
                return &handlers[i];
        }
        return nullptr;
    }

    /**
     * @brief Unsubscribes a HandlerEntry from all Tags it was registered to.
     *
     * For each Tag in the entry’s tagList, removes the entry pointer from
     * the Tag’s handlers array and compacts the array to close gaps.
     *
     * @param entry Pointer to the HandlerEntry to remove from all tags.
     */
    inline void HandlerManager::unsubscribeEntryFromTags(HandlerEntry *entry)
    {
        for (size_t t = 0; t < entry->tagCount; ++t)
        {
            Tag *tag = const_cast<Tag *>(entry->tagList[t]);
            size_t wr = 0;
            for (size_t rd = 0; rd < tag->handlerCount; ++rd)
            {
                if (tag->handlers[rd] != entry)
                    tag->handlers[wr++] = tag->handlers[rd];
            }
            tag->handlerCount = wr;
        }
    }

    /**
     * @brief Removes a handler slot from the internal array by index.
     *
     * Shifts all entries after @p index one position left to fill the gap,
     * then decrements the handler count.
     *
     * @param index Index of the handler slot to remove.
     */
    inline void HandlerManager::compactHandlerArray(size_t index)
    {
        for (size_t i = index + 1; i < handlerCount; ++i)
        {
            handlers[i - 1] = handlers[i];
        }
        --handlerCount;
    }

    /**
     * @brief Registers a handler for a list of Tag* subscriptions.
     *
     * Convenience wrapper that checks capacity, creates the entry,
     * and subscribes it to the provided tags.
     *
     * @param level     Minimum log level threshold for this handler.
     * @param fn        Callback function pointer to invoke on log dispatch.
     * @param ctx       User-supplied context passed to the callback.
     * @param tagList   Array of Tag* pointers this handler should subscribe to.
     * @param tagCount  Number of tags in @p tagList.
     * @param name      Optional human-readable name for diagnostics or removal.
     * @return true if registration succeeded, false if capacity is exceeded.
     */
    inline bool HandlerManager::registerHandlerForTags(LogLevel level,
                                                       LogHandler fn,
                                                       void *ctx,
                                                       const Tag *tagList[],
                                                       size_t tagCount,
                                                       const char *name)
    {
        if (!hasCapacity())
            return false;
        auto &entry = createEntry(level, fn, ctx, tagList, tagCount, name);
        subscribeEntryToTags(&entry, tagList, tagCount);
        return true;
    }

    /**
     * @brief Deletes a handler by its unique ID.
     *
     * Finds the entry by ID, unsubscribes it from all Tags, and compacts
     * the internal handler array.
     *
     * @param id Unique identifier of the handler to delete.
     * @return true if the handler was found and deleted; false otherwise.
     */
    inline bool HandlerManager::deleteHandlerByID(uint16_t id)
    {
        auto *e = findEntryByID(id);
        if (!e)
            return false;
        unsubscribeEntryFromTags(e);
        compactHandlerArray(static_cast<size_t>(e - handlers));
        return true;
    }

    /**
     * @brief Deletes a handler by its name
     *
     * Finds the entry by name, unsubscribes it from all Tags, and compacts
     * the internal handler array.
     *
     * @param name Name of the handler to delete.
     * @return true if the handler was found and deleted; false otherwise.
     */
    inline bool HandlerManager::deleteHandlerByName(const char *name)
    {
        auto *e = findEntryByName(name);
        if (!e)
            return false;
        unsubscribeEntryFromTags(e);
        compactHandlerArray(static_cast<size_t>(e - handlers));
        return true;
    }
}; // namespace LogAnywhere
