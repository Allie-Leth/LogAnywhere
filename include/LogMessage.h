#pragma once

/**
 * @file LogMessage.h
 * @brief Defines the LogMessage structure passed to all registered handlers.
 *
 * A `LogMessage` contains all the core information about a log event, including
 * its severity, origin (tag), content, and optionally, a timestamp.
 */

#include <cstdint>
#include "LogLevel.h"

namespace LogAnywhere {

    /**
     * @brief Describes a single log event passed to handlers.
     *
     * Each handler receives a `LogMessage` and may route it, serialize it,
     * display it, or store it depending on its configuration.
     */
    struct LogMessage {
        LogLevel level;         ///< Severity level of the log message
        const char* tag;        ///< Subsystem or component name (e.g., "WiFi", "OTA", etc.)
        const char* message;    ///< Already-formatted message string
        uint64_t timestamp = 0; ///< Optional timestamp (UTC or monotonic); injected externally
    };

} // namespace LogAnywhere
