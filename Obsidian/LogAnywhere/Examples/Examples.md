
This section contains practical usage examples for integrating and extending the LogAnywhere library in embedded, desktop, or test environments.

---

## Basic Usage

### Registering a Logger

- Create a `Logger` instance
```C++
int main() {
    auto consoleHandler = []( const LogAnywhere::LogMessage &msg, void *ctx ) {
        auto *out = static_cast<std::ostream*>(ctx);
        *out << "[" 
             << LogAnywhere::toString(msg.level) 
             << "] " 
             << msg.tag 
             << ": " 
             << msg.message 
             << "\n";
    };

    // Register console handler at INFO+
    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::INFO,
        consoleHandler,
        &std::cout
    );

    // Send a basic log
    LogAnywhere::log(
        LogAnywhere::LogLevel::INFO,
        "MAIN",
        "System initialized"
    );
}
```
- Register a basic handler (e.g., `std::cout`, Serial)
- Make a simple `log()` call

### Logging with Different Severity Levels

- Demonstrate use of:
  - `LogLevel::TRACE`
  - `LogLevel::DEBUG`
  - `LogLevel::INFO`
  - `LogLevel::WARN`
  - `LogLevel::ERROR`
- Show how severity filtering works

### Logging with `logf()`

- Use `logf(LogLevel, tag, format, ...)` to log dynamic values
	```
	Not Implemented
	```

- Printf-style formatting example
	```
	Not Implemented
	```



## Handler Patterns

### Logger Severity Levels

```C++
int main() {
    auto consoleHandler = []( const LogAnywhere::LogMessage &msg, void *ctx ) {
        auto *out = static_cast<std::ostream*>(ctx);
        *out << "[" 
             << LogAnywhere::toString(msg.level) 
             << "] " 
             << msg.tag 
             << ": " 
             << msg.message 
             << "\n";
    };

    // Only WARN+ go to this handler
    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::WARN,
        consoleHandler,
        &std::cout
    );

    LogAnywhere::log(LogAnywhere::LogLevel::TRACE, "TEST", "Trace message");   // Ignored
    LogAnywhere::log(LogAnywhere::LogLevel::DEBUG, "TEST", "Debug message");   // Ignored
    LogAnywhere::log(LogAnywhere::LogLevel::INFO,  "TEST", "Info message");    // Ignored
    LogAnywhere::log(LogAnywhere::LogLevel::WARN,  "TEST", "Warn message");    // Printed
    LogAnywhere::log(LogAnywhere::LogLevel::ERR,   "TEST", "Error message");   // Printed
}
```


### Multi-handler Severity Levels

```C++
int main() {
    std::ostringstream mqttStream;
    std::ostringstream serialStream;

    auto mqttHandler = []( const LogAnywhere::LogMessage &msg, void *ctx ) {
        auto *out = static_cast<std::ostringstream*>(ctx);
        *out << "[MQTT] " 
             << LogAnywhere::toString(msg.level) 
             << " - " 
             << msg.tag 
             << ": " 
             << msg.message 
             << "\n";
    };

    auto serialHandler = []( const LogAnywhere::LogMessage &msg, void *ctx ) {
        auto *out = static_cast<std::ostringstream*>(ctx);
        *out << "[SERIAL] " 
             << LogAnywhere::toString(msg.level) 
             << " - " 
             << msg.tag 
             << ": " 
             << msg.message 
             << "\n";
    };

    LogAnywhere::registerHandler(LogAnywhere::LogLevel::INFO, mqttHandler,   &mqttStream);
    LogAnywhere::registerHandler(LogAnywhere::LogLevel::ERR,  serialHandler, &serialStream);

    LogAnywhere::log(LogAnywhere::LogLevel::DEBUG, "NET", "Network check");          // Ignored
    LogAnywhere::log(LogAnywhere::LogLevel::INFO,  "NET", "Ping successful");        // MQTT only
    LogAnywhere::log(LogAnywhere::LogLevel::ERR,   "NET", "Failed to reach server"); // Both

    std::cout << "=== MQTT Output ===\n"   << mqttStream.str();
    std::cout << "=== Serial Output ===\n" << serialStream.str();
}
```
---

## Filters and Context

### Tag-Based Filtering

```C++
int main() {
    std::ostringstream filteredStream;
    std::ostringstream unfilteredStream;

    auto tagFilter = []( const char* tag, void* ) -> bool {
        return std::string(tag) == "SENSOR";
    };

    auto filteredHandler = []( const LogAnywhere::LogMessage &msg, void *ctx ) {
        auto *out = static_cast<std::ostringstream*>(ctx);
        *out << "[FILTERED] " 
             << LogAnywhere::toString(msg.level) 
             << " - " 
             << msg.tag 
             << ": " 
             << msg.message 
             << "\n";
    };

    auto unfilteredHandler = []( const LogAnywhere::LogMessage &msg, void *ctx ) {
        auto *out = static_cast<std::ostringstream*>(ctx);
        *out << "[RAW] " 
             << LogAnywhere::toString(msg.level) 
             << " - " 
             << msg.tag 
             << ": " 
             << msg.message 
             << "\n";
    };

    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::INFO,
        filteredHandler,
        &filteredStream,
        tagFilter
    );
    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::INFO,
        unfilteredHandler,
        &unfilteredStream
    );

    LogAnywhere::log(LogAnywhere::LogLevel::INFO, "SENSOR", "Temperature: 22.5C");
    LogAnywhere::log(LogAnywhere::LogLevel::INFO, "CORE",   "System ready");
    LogAnywhere::log(LogAnywhere::LogLevel::INFO, "SENSOR", "Humidity: 48%");

    std::cout << "=== Filtered (Only SENSOR) ===\n"   << filteredStream.str();
    std::cout << "=== Unfiltered (All INFO+) ===\n" << unfilteredStream.str();
}
```

##### Expected Output
```C++
=== Filtered Output (Only SENSOR) ===
[FILTERED] INFO - SENSOR: Temperature: 22.5C
[FILTERED] INFO - SENSOR: Humidity: 48%

=== Unfiltered Output (All INFO+) ===
[RAW] INFO - SENSOR: Temperature: 22.5C
[RAW] INFO - CORE: System ready
[RAW] INFO - SENSOR: Humidity: 48%

```
### Passing Context into Handlers

```c++
int main() {
    // Create a standalone Logger instance
    LogAnywhere::Logger logger;

    std::ostringstream consoleStream;
    std::vector<std::string> asyncQueue;

    // A generic handler that writes to an ostream when ctx is an ostringstream
    auto ContextAwareHandler = [](
        const LogAnywhere::LogMessage &msg,
        void *ctx
    ) {
        if (auto *out = static_cast<std::ostringstream*>(ctx)) {
            *out << "["
                 << LogAnywhere::toString(msg.level)
                 << "] "
                 << msg.tag
                 << ": "
                 << msg.message
                 << "\n";
        }
    };

    // An async-style handler that pushes messages into a string vector
    auto AsyncHandler = [](
        const LogAnywhere::LogMessage &msg,
        void *ctx
    ) {
        if (auto *queue = static_cast<std::vector<std::string>*>(ctx)) {
            queue->push_back(
                std::string(msg.tag) + ": " + msg.message
            );
        }
    };

    // Register both handlers at INFO level, passing their respective contexts
    logger.registerHandler(
        LogAnywhere::LogLevel::INFO,
        ContextAwareHandler,
        &consoleStream
    );
    logger.registerHandler(
        LogAnywhere::LogLevel::INFO,
        AsyncHandler,
        &asyncQueue
    );

    // Emit some logs
    logger.log(LogAnywhere::LogLevel::INFO, "SYS", "Boot complete");
    logger.log(LogAnywhere::LogLevel::INFO, "NET", "Connected to WiFi");

    // Output the results
    std::cout << "=== Stream Output ===\n"
              << consoleStream.str() << "\n";

    std::cout << "=== Async Queue ===\n";
    for (const auto &entry : asyncQueue) {
        std::cout << entry << "\n";
    }

    return 0;
}
```

##### Expected Output
```C++
=== Stream Output ===
[INFO] SYS: Boot complete
[INFO] NET: Connected to WiFi

=== Async Queue ===
SYS: Boot complete
NET: Connected to WiFi

```
### Named Handlers

```C++

int main() {
    LogAnywhere::Logger logger;
    std::ostringstream alertStream;

    // A handler that sends alerts to a Twilio-like sink
    auto TwilioHandler = [](
        const LogAnywhere::LogMessage &msg,
        void *ctx
    ) {
        if (auto *out = static_cast<std::ostringstream*>(ctx)) {
            *out << "[TWILIO] "
                 << msg.tag
                 << ": "
                 << msg.message
                 << "\n";
        }
    };

    // Register a named handler called "Twilio"
    logger.registerHandler(
        LogAnywhere::LogLevel::ERR,
        TwilioHandler,
        &alertStream,
        nullptr,
        "Twilio"
    );

    // Only error-level logs go through
    logger.log(LogAnywhere::LogLevel::ERR, "ALERT", "Battery level critical");

    std::cout << "=== Named Handler Output ===\n"
              << alertStream.str();
}
```

##### Expected Output
```C++
=== Named Handler Output ===
[TWILIO] ALERT: Battery level critical

```

---

## Time and Order

### Using Default Timestamp (Sequential counter fallback)
```C++
int main() {
    std::ostringstream out;

    auto simpleHandler = [](
        const LogAnywhere::LogMessage &msg,
        void *ctx
    ) {
        if (auto *stream = static_cast<std::ostringstream*>(ctx)) {
            *stream << "[" 
                    << msg.timestamp 
                    << "] "
                    << LogAnywhere::toString(msg.level) 
                    << " - "
                    << msg.tag
                    << ": "
                    << msg.message
                    << "\n";
        }
    };

    // Using the free-function API
    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::INFO,
        simpleHandler,
        &out
    );

    LogAnywhere::log(LogAnywhere::LogLevel::INFO, "BOOT", "Logger starting up");
    LogAnywhere::log(LogAnywhere::LogLevel::INFO, "BOOT", "Initialization complete");

    std::cout << out.str();
}
```

##### Expected Output
```C++
[1] INFO - BOOT: Logger starting up
[2] INFO - BOOT: Initialization complete
```
### Providing a Custom Timestamp Function

```C++
int main() {
    std::ostringstream out;

    // Provide microsecond timestamps from the system clock
    auto systemClockTime = []() -> uint64_t {
        using namespace std::chrono;
        return duration_cast<microseconds>(
            system_clock::now().time_since_epoch()
        ).count();
    };

    // Set on the static logger instance
    LogAnywhere::logger.setTimestampProvider(systemClockTime);

    auto handler = [](
        const LogAnywhere::LogMessage &msg,
        void *ctx
    ) {
        if (auto *stream = static_cast<std::ostringstream*>(ctx)) {
            *stream << "[" 
                    << msg.timestamp 
                    << "] "
                    << LogAnywhere::toString(msg.level) 
                    << " - "
                    << msg.tag 
                    << ": " 
                    << msg.message 
                    << "\n";
        }
    };

    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::INFO,
        handler,
        &out
    );

    LogAnywhere::log(
        LogAnywhere::LogLevel::INFO,
        "RTC",
        "Synchronized with system clock"
    );

    std::cout << out.str();
}
```
##### Expected Output
```c++
[1745201789334567] INFO - RTC: Synchronized with system clock
```

---

## Advanced Topics

### Async Logging 

- **Not Implemented**

### Extending LogLevel Enum (Planned)

- **Not Implemented**

### Minimal Integration for Embedded

- Include only `LogAnywhere.h` and avoid dynamic allocation
- Register handler using raw function pointer + `context`

---

