
This section contains practical usage examples for integrating and extending the LogAnywhere library in embedded, desktop, or test environments.

---

## Basic Usage

### Registering a Logger

- Create a `Logger` instance
```C++
#include "LogAnywhere.h"
#include <iostream>

using namespace LogAnywhere;

int main() {
    auto consoleHandler = [](const LogMessage& msg, void* ctx) {
        auto* out = static_cast<std::ostream*>(ctx);
        *out << "[" << toString(msg.level) << "] "
             << msg.tag << ": " << msg.message << "\n";
    };

    // Register console handler
    registerHandler(LogLevel::INFO, consoleHandler, &std::cout);

    // Send a basic log
    log(LogLevel::INFO, "MAIN", "System initialized");
}

``
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

---

## Handler Patterns

### Logger Severity Levels

```C++
#include "LogAnywhere.h"
#include <iostream>

using namespace LogAnywhere;

int main() {
    auto consoleHandler = [](const LogMessage& msg, void* ctx) {
        auto* out = static_cast<std::ostream*>(ctx);
        *out << "[" << toString(msg.level) << "] "
             << msg.tag << ": " << msg.message << "\n";
    };

    registerHandler(LogLevel::WARN, consoleHandler, &std::cout);

    log(LogLevel::TRACE, "TEST", "Trace message");   // Ignored
    log(LogLevel::DEBUG, "TEST", "Debug message");   // Ignored
    log(LogLevel::INFO,  "TEST", "Info message");    // Ignored
    log(LogLevel::WARN,  "TEST", "Warn message");    // Printed
    log(LogLevel::ERR,   "TEST", "Error message");   // Printed
}


```


### Multi-handler Severity Levels

```C++
#include "LogAnywhere.h"
#include <iostream>
#include <sstream>

using namespace LogAnywhere;

int main() {
    std::ostringstream mqttStream;
    std::ostringstream serialStream;

    auto mqttHandler = [](const LogMessage& msg, void* ctx) {
        auto* out = static_cast<std::ostringstream*>(ctx);
        *out << "[MQTT] " << toString(msg.level) << " - "
             << msg.tag << ": " << msg.message << "\n";
    };

    auto serialHandler = [](const LogMessage& msg, void* ctx) {
        auto* out = static_cast<std::ostringstream*>(ctx);
        *out << "[SERIAL] " << toString(msg.level) << " - "
             << msg.tag << ": " << msg.message << "\n";
    };

    registerHandler(LogLevel::INFO, mqttHandler, &mqttStream);
    registerHandler(LogLevel::ERR, serialHandler, &serialStream);

    log(LogLevel::DEBUG, "NET", "Network check");          // Ignored by both
    log(LogLevel::INFO,  "NET", "Ping successful");        // MQTT only
    log(LogLevel::ERR,   "NET", "Failed to reach server"); // Both

    std::cout << "=== MQTT Output ===\n" << mqttStream.str();
    std::cout << "=== Serial Output ===\n" << serialStream.str();
}


```
---

## Filters and Context

### Tag-Based Filtering

```C++
#include "LogAnywhere.h"
#include <iostream>
#include <sstream>

using namespace LogAnywhere;

int main() {
    std::ostringstream filteredStream;
    std::ostringstream unfilteredStream;

    auto tagFilter = [](const char* tag, void*) -> bool {
        return std::string(tag) == "SENSOR";
    };

    auto filteredHandler = [](const LogMessage& msg, void* ctx) {
        auto* out = static_cast<std::ostringstream*>(ctx);
        *out << "[FILTERED] " << toString(msg.level) << " - "
             << msg.tag << ": " << msg.message << "\n";
    };

    auto unfilteredHandler = [](const LogMessage& msg, void* ctx) {
        auto* out = static_cast<std::ostringstream*>(ctx);
        *out << "[RAW] " << toString(msg.level) << " - "
             << msg.tag << ": " << msg.message << "\n";
    };

    registerHandler(LogLevel::INFO, filteredHandler, &filteredStream, tagFilter);
    registerHandler(LogLevel::INFO, unfilteredHandler, &unfilteredStream);

    log(LogLevel::INFO, "SENSOR", "Temperature: 22.5C");
    log(LogLevel::INFO, "CORE", "System ready");
    log(LogLevel::INFO, "SENSOR", "Humidity: 48%");

    std::cout << "=== Filtered Output (Only SENSOR) ===\n" << filteredStream.str();
    std::cout << "=== Unfiltered Output (All INFO+) ===\n" << unfilteredStream.str();
}


```

#### Expected Output
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
#include "LogAnywhere.h"
#include <iostream>
#include <sstream>
#include <vector>

using namespace LogAnywhere;

int main() {
    Logger logger;

    std::ostringstream consoleStream;
    std::vector<std::string> asyncQueue;

    // A generic handler that writes to either ostream or queue, based on context
    auto ContextAwareHandler = [](const LogMessage& msg, void* ctx) {
        if (auto* stream = static_cast<std::ostringstream*>(ctx)) {
            *stream << "[" << toString(msg.level) << "] "
                    << msg.tag << ": " << msg.message << "\n";
        }
    };

    auto AsyncHandler = [](const LogMessage& msg, void* ctx) {
        auto* queue = static_cast<std::vector<std::string>*>(ctx);
        queue->push_back(std::string(msg.tag) + ": " + msg.message);
    };

    // Register same handler logic for different outputs
    logger.registerHandler(LogLevel::INFO, ContextAwareHandler, &consoleStream);
    logger.registerHandler(LogLevel::INFO, AsyncHandler, &asyncQueue);

    logger.log(LogLevel::INFO, "SYS", "Boot complete");
    logger.log(LogLevel::INFO, "NET", "Connected to WiFi");

    std::cout << "=== Stream Output ===\n" << consoleStream.str();

    std::cout << "=== Async Queue ===\n";
    for (const auto& entry : asyncQueue) {
        std::cout << entry << "\n";
    }
}

```

#### Expected Output
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
#include <iostream>
#include <sstream>

using namespace LogAnywhere;

int main() {
    Logger logger;
    std::ostringstream alertStream;

    auto TwilioHandler = [](const LogMessage& msg, void* ctx) {
        auto* out = static_cast<std::ostringstream*>(ctx);
        *out << "[TWILIO] " << msg.tag << ": " << msg.message << "\n";
    };

    // Register a named handler called "Twilio"
    logger.registerHandler(LogLevel::ERR, TwilioHandler, &alertStream, "Twilio");

    logger.log(LogLevel::ERR, "ALERT", "Battery level critical");

    std::cout << "=== Named Handler Output ===\n";
    std::cout << alertStream.str();
}

```

#### Output
```C++
=== Named Handler Output ===
[TWILIO] ALERT: Battery level critical

```

---

## Time and Order

### Using Default Timestamp (Sequential counter fallback)
```C++
#include "LogAnywhere.h"
#include <iostream>
#include <sstream>

using namespace LogAnywhere;

int main() {
    std::ostringstream out;

    auto simpleHandler = [](const LogMessage& msg, void* ctx) {
        auto* stream = static_cast<std::ostringstream*>(ctx);
        *stream << "[" << msg.timestamp << "] "
                << toString(msg.level) << " - "
                << msg.tag << ": " << msg.message << "\n";
    };

    registerHandler(LogLevel::INFO, simpleHandler, &out);

    log(LogLevel::INFO, "BOOT", "Logger starting up");
    log(LogLevel::INFO, "BOOT", "Initialization complete");

    std::cout << out.str();
}


```

#### 
```C++
[1] INFO - BOOT: Logger starting up
[2] INFO - BOOT: Initialization complete
```
### Providing a Custom Timestamp Function

```C++
#include "LogAnywhere.h"
#include <iostream>
#include <sstream>
#include <chrono>

using namespace LogAnywhere;

int main() {
    std::ostringstream out;

    auto systemClockTime = []() -> uint64_t {
        using namespace std::chrono;
        return duration_cast<microseconds>(
            system_clock::now().time_since_epoch()
        ).count();
    };

    logger.setTimestampProvider(systemClockTime);

    auto handler = [](const LogMessage& msg, void* ctx) {
        auto* stream = static_cast<std::ostringstream*>(ctx);
        *stream << "[" << msg.timestamp << "] "
                << toString(msg.level) << " - "
                << msg.tag << ": " << msg.message << "\n";
    };

    registerHandler(LogLevel::INFO, handler, &out);

    log(LogLevel::INFO, "RTC", "Synchronized with system clock");

    std::cout << out.str();
}
```

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

