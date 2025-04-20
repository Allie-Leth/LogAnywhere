#pragma once

/**
 * @file LogLevel.h
 * @brief Defines standard log levels for use with LogAnywhere.
 *
 * This file contains the `LogLevel` enumeration representing severity levels,
 * along with a helper function to convert levels to human-readable strings.
 */

#include <cstdint>

namespace LogAnywhere {

    /**
     * @brief Standard log levels for routing and filtering log messages.
     *
     * These levels follow conventional logging practices. Handlers may be registered
     * to respond to one or more levels starting from a threshold.
     */
    enum class LogLevel : uint8_t {
        TRACE = 0,  ///< Extremely fine-grained logs, usually disabled by default
        DEBUG,      ///< Debugging details useful for developers
        INFO,       ///< General informational messages
        WARN,       ///< Potential issues or recoverable problems
        ERROR       ///< Serious issues requiring attention
    };

    /**
     * @brief Converts a `LogLevel` enum to a human-readable string.
     *
     * @param level The log level to convert
     * @return A constant string representing the level
     */
    inline const char* toString(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            default:              return "UNKNOWN";
        }
    }

} // namespace LogAnywhere

// TODO: Allow custom log levels via extension mechanism
// TODO: Make toString() overridable (e.g., via function pointer hook or constexpr map)
