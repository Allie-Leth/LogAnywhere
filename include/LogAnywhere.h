#pragma once

/**
 * @file LogAnywhere.h
 * @brief Public-facing API header for the LogAnywhere logging framework.
 *
 * This header exposes the core `Logger` class and associated types for integrating
 * with the logging system. All consumers of the library should include only this file.
 */

#include "LogLevel.h"
#include "LogMessage.h"

namespace LogAnywhere {

    /**
     * @brief Log handler function signature.
     *
     * A handler is any user-defined function that accepts a `LogMessage` and an
     * optional context pointer. These are registered using `Logger::registerHandler()`.
     */
    using LogHandler = void(*)(const LogMessage&, void* context);

    /**
     * @brief Forward declaration of the core Logger class.
     *
     * The full definition is provided in `Logger.h`, which is included internally.
     * This header acts as the external interface, so implementation details are hidden.
     */
    class Logger;

}
