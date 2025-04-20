#pragma once

#include "LogLevel.h"
#include "LogMessage.h"

namespace LogAnywhere {

    /**
     * @brief Log handler function signature.
     * Called with a fully populated LogMessage and optional user context.
     */
    using LogHandler = void(*)(const LogMessage&, void* context);

    /**
     * @brief Optional filter function for tag-based routing.
     * If provided, the handler is only triggered when this returns true.
     */
    using TagFilterFn = bool(*)(const char* tag, void* context);

    /**
     * @brief Represents a registered handler with routing metadata.
     */
    struct HandlerEntry {
        uint16_t id;                ///< Unique internal identifier
        const char* name;           ///< Optional human-readable name (e.g., "MQTT", "Twilio")
        LogLevel level;             ///< Minimum log level to trigger this handler
        LogHandler handler;         ///< Function pointer to the logging routine
        void* context;              ///< Arbitrary context pointer passed to the handler
        TagFilterFn tagFilter;      ///< Optional tag filter function
        void* filterContext;        ///< Optional context for the tag filter

        HandlerEntry() = default;

        HandlerEntry(uint16_t id,
                     const char* name,
                     LogLevel level,
                     LogHandler handler,
                     void* context,
                     TagFilterFn tagFilter = nullptr,
                     void* filterContext = nullptr)
            : id(id),
              name(name),
              level(level),
              handler(handler),
              context(context),
              tagFilter(tagFilter),
              filterContext(filterContext) {}
    };

} // namespace LogAnywhere
