
This section contains practical usage examples for integrating and extending the LogAnywhere library in embedded, desktop, or test environments.

---

## Basic Usage

### Registering a Logger

- Create a `Logger` instance
```C++
// Basic Usage: Registering a Logger
int main() {
    static LogAnywhere::Tag TAG_APP("APP");
    const LogAnywhere::Tag* tags[] = { &TAG_APP };

    auto consoleHandler = [](const LogAnywhere::LogMessage& m, void*) {
        std::printf("[%s] %s: %s\n",
            LogAnywhere::toString(m.level),
            m.tag,
            m.message);
    };

    // Subscribe consoleHandler at INFO+
    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::INFO,
        consoleHandler,
        nullptr,
        tags,
        1
    );

    // Emit a log
    LogAnywhere::log(
        LogAnywhere::LogLevel::INFO,
        &TAG_APP,
        "System initialized"
    );
    return 0;
}


```

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
    static LogAnywhere::Tag TAG_NET("NET");
    const LogAnywhere::Tag* tags[] = { &TAG_NET };

    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::DEBUG,
        [](const LogAnywhere::LogMessage& m, void*) {
            std::printf("[%s] %s: %s\n",
                LogAnywhere::toString(m.level),
                m.tag,
                m.message);
        },
        nullptr,
        tags,
        1
    );

    LogAnywhere::log(LogAnywhere::LogLevel::TRACE, &TAG_NET, "Trace message");  // Ignored
    LogAnywhere::log(LogAnywhere::LogLevel::DEBUG, &TAG_NET, "Debug message");  // Printed
    LogAnywhere::log(LogAnywhere::LogLevel::INFO,  &TAG_NET, "Info message");   // Printed
    LogAnywhere::log(LogAnywhere::LogLevel::WARN,  &TAG_NET, "Warn message");   // Printed
    LogAnywhere::log(LogAnywhere::LogLevel::ERR,   &TAG_NET, "Error message");  // Printed

    return 0;
}
```


### Multi-handler Severity Levels

```C++
int main() {
    static LogAnywhere::Tag TAG_IO("IO");
    const LogAnywhere::Tag* tags[] = { &TAG_IO };

    std::ostringstream mqttBuf, serialBuf;

    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::INFO,
        [](const LogAnywhere::LogMessage& m, void* ctx) {
            auto& out = *static_cast<std::ostringstream*>(ctx);
            out << "[MQTT][" << LogAnywhere::toString(m.level)
                << "] " << m.tag << ": " << m.message << "\n";
        },
        &mqttBuf,
        tags,
        1
    );

    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::ERR,
        [](const LogAnywhere::LogMessage& m, void* ctx) {
            auto& out = *static_cast<std::ostringstream*>(ctx);
            out << "[SERIAL][" << LogAnywhere::toString(m.level)
                << "] " << m.tag << ": " << m.message << "\n";
        },
        &serialBuf,
        tags,
        1
    );

    LogAnywhere::log(LogAnywhere::LogLevel::INFO, &TAG_IO, "Operation OK");
    LogAnywhere::log(LogAnywhere::LogLevel::ERR,  &TAG_IO, "Operation failed");

    std::printf("MQTT log:\n%s", mqttBuf.str().c_str());
    std::printf("Serial log:\n%s", serialBuf.str().c_str());
    return 0;
}

```
---

## Filters and Context

### Tag-Based Filtering

```C++
int main() {
    static LogAnywhere::Tag TAG_SENSOR("SENSOR");
    static LogAnywhere::Tag TAG_CORE("CORE");
    const LogAnywhere::Tag* sensorTags[] = { &TAG_SENSOR };
    const LogAnywhere::Tag* allTags[]    = { &TAG_SENSOR, &TAG_CORE };

    std::ostringstream filtBuf, rawBuf;

    // Only SENSOR messages
    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::INFO,
        [](const LogAnywhere::LogMessage& m, void* ctx) {
            auto& out = *static_cast<std::ostringstream*>(ctx);
            out << "[FILT] " << m.tag << ": " << m.message << "\n";
        },
        &filtBuf,
        sensorTags,
        1
    );

    // All INFO+ messages
    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::INFO,
        [](const LogAnywhere::LogMessage& m, void* ctx) {
            auto& out = *static_cast<std::ostringstream*>(ctx);
            out << "[RAW] " << m.tag << ": " << m.message << "\n";
        },
        &rawBuf,
        allTags,
        2
    );

    LogAnywhere::log(LogAnywhere::LogLevel::INFO, &TAG_SENSOR, "Temp=22.5C");
    LogAnywhere::log(LogAnywhere::LogLevel::INFO, &TAG_CORE,   "Ready");
    LogAnywhere::log(LogAnywhere::LogLevel::INFO, &TAG_SENSOR, "Hum=48%");

    std::printf("Filtered:\n%s", filtBuf.str().c_str());
    std::printf("Raw:\n%s", rawBuf.str().c_str());
    return 0;
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
#### Registering a handler for multiple tags and logging to them
```C++
int main() {
    // Define two tags
    static LogAnywhere::Tag TAG_SENSOR("SENSOR");
    static LogAnywhere::Tag TAG_STATUS("STATUS");
    const LogAnywhere::Tag* tags[] = { &TAG_SENSOR, &TAG_STATUS };

    // Buffer to capture handler output
    std::ostringstream out;

    // Handler invoked for both SENSOR and STATUS logs
    auto multiTagHandler = [](const LogAnywhere::LogMessage& msg, void* ctx) {
        auto& os = *static_cast<std::ostringstream*>(ctx);
        os << "[" << LogAnywhere::toString(msg.level) << "] "
           << msg.tag << ": " << msg.message << "\n";
    };

    // Register the handler at INFO level for both tags
    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::INFO,
        multiTagHandler,
        &out,
        tags,
        2
    );

    // Emit logs to each tag
    LogAnywhere::log(LogAnywhere::LogLevel::INFO, &TAG_SENSOR, "Temperature = 21.3°C");
    LogAnywhere::log(LogAnywhere::LogLevel::INFO, &TAG_STATUS, "System OK");
    LogAnywhere::log(LogAnywhere::LogLevel::DEBUG, &TAG_SENSOR, "Raw ADC = 1023"); // Ignored (below INFO)

    // Display captured output
    std::cout << out.str();
    return 0;
}
```
##### Expected Output
```C++
[INFO] SENSOR: Temperature = 21.3°C
[INFO] STATUS: System OK
```
### Passing Context into Handlers

```c++
int main() {
    static LogAnywhere::Tag TAG_SYS("SYS");
    const LogAnywhere::Tag* tags[] = { &TAG_SYS };

    std::ostringstream syncOut;
    std::vector<std::string> queue;

    // Sync stream
    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::INFO,
        [](const LogAnywhere::LogMessage& m, void* ctx) {
            auto& out = *static_cast<std::ostringstream*>(ctx);
            out << m.tag << ": " << m.message << "\n";
        },
        &syncOut,
        tags,
        1
    );

    // Async queue
    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::INFO,
        [](const LogAnywhere::LogMessage& m, void* ctx) {
            auto& q = *static_cast<std::vector<std::string>*>(ctx);
            q.push_back(std::string(m.tag) + ": " + m.message);
        },
        &queue,
        tags,
        1
    );

    LogAnywhere::log(LogAnywhere::LogLevel::INFO, &TAG_SYS, "Boot");
    LogAnywhere::log(LogAnywhere::LogLevel::INFO, &TAG_SYS, "Run");

    std::printf("Sync:\n%s", syncOut.str().c_str());
    std::puts("Async queue:");
    for (auto& e : queue) std::puts(e.c_str());
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
#include "LogAnywhere.h"
#include <sstream>
#include <cstdio>

int main() {
    static LogAnywhere::Tag TAG_ALERT("ALERT");
    const LogAnywhere::Tag* tags[] = { &TAG_ALERT };

    std::ostringstream alertBuf;

    // Named handler "Twilio"
    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::ERR,
        [](const LogAnywhere::LogMessage& m, void* ctx) {
            auto& out = *static_cast<std::ostringstream*>(ctx);
            out << "[TWILIO] " << m.message << "\n";
        },
        &alertBuf,
        tags,
        1,
        "Twilio"
    );

    LogAnywhere::log(LogAnywhere::LogLevel::ERR, &TAG_ALERT, "Battery low");

    std::printf("Named handler output:\n%s", alertBuf.str().c_str());
    return 0;
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
#include "LogAnywhere.h"
#include <sstream>
#include <cstdio>

int main() {
    static LogAnywhere::Tag TAG_BOOT("BOOT");
    const LogAnywhere::Tag* tags[] = { &TAG_BOOT };

    std::ostringstream out;
    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::INFO,
        [](const LogAnywhere::LogMessage& m, void* ctx) {
            auto& os = *static_cast<std::ostringstream*>(ctx);
            os << "[" << m.timestamp << "] "
               << m.tag << ": " << m.message << "\n";
        },
        &out,
        tags,
        1
    );

    LogAnywhere::log(LogAnywhere::LogLevel::INFO, &TAG_BOOT, "Start");
    LogAnywhere::log(LogAnywhere::LogLevel::INFO, &TAG_BOOT, "Ready");

    std::printf("%s", out.str().c_str());
    return 0;
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
    // Hook into system clock (microseconds)
    LogAnywhere::logger.setTimestampProvider([]() -> uint64_t {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    });

    static LogAnywhere::Tag TAG_RTC("RTC");
    const LogAnywhere::Tag* tags[] = { &TAG_RTC };

    std::ostringstream out;
    LogAnywhere::registerHandler(
        LogAnywhere::LogLevel::INFO,
        [](const LogAnywhere::LogMessage& m, void* ctx) {
            auto& os = *static_cast<std::ostringstream*>(ctx);
            os << "[" << m.timestamp << "] "
               << m.tag << ": " << m.message << "\n";
        },
        &out,
        tags,
        1
    );

    LogAnywhere::log(LogAnywhere::LogLevel::INFO, &TAG_RTC, "Synced");
    std::printf("%s", out.str().c_str());
    return 0;
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

