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

    struct LogMessage {

        LogMessage() = default; ///< Default constructor
        

        LogLevel     level;      ///< Severity level of the log message
        const char*  tag;        ///< Subsystem or component name
        const char*  message;    ///< Already-formatted message string
        uint64_t     timestamp;  ///< Optional timestamp

        constexpr LogMessage(LogLevel lvl,
                             const char* tg,
                             const char* msg,
                             uint64_t ts = 0)
          : level(lvl),
            tag(tg),
            message(msg),
            timestamp(ts)
        {}
    };
} // namespace LogAnywhere
