#pragma once
#include "LogLevel.h"
#include "LogMessage.h"

namespace LogAnywhere {

    using LogHandler = void(*)(const LogMessage&, void* context);
    using TagFilterFn = bool(*)(const char* tag, void* context);

    struct HandlerEntry {
        uint16_t id = 0;
        const char* name = nullptr;
        LogLevel level;
        LogHandler handler;
        void* context = nullptr;

        TagFilterFn tagFilter = nullptr;
        void* filterContext = nullptr;
    };

} // namespace LogAnywhere
