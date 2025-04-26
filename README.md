# LogAnywhere

**LogAnywhere** is a lightweight, backend-agnostic logging library for embedded C++ development. It focuses on minimalism, clarity, and total decoupling from output protocols. The result is a flexible log router that works across microcontrollers, Linux targets, and constrained environments. 

---

## Why LogAnywhere Exists

LogAnywhere was born out of the needs of a growing ESP32 and Pi fleet running complex mesh and mesh-of-mesh networks - particularly in CI/CD environments. Traditional logging frameworks quickly proved inadequate when simultaneous outputs—to a file, serial console, MQTT broker, or custom sink—became essential. After repeatedly reimplementing the same handlers by hand, I created LogAnywhere as a single, reusable core. It cleanly decouples routing from output, enabling you to plug in any combination of handlers across your entire fleet.

In embedded systems, logging is far more than development-time verbosity—it's often part of the device's primary functionality. LogAnywhere is designed from the ground up to support multi-destination logging as a fundamental feature, not an afterthought.

You choose the destinations. LogAnywhere routes each entry based on severity, tags, and optional filters. It remains completely transport and format agnostic, giving you full control over how and where your logs are delivered.

---

## Key Features

- Embedded design focused - Extremely low overhead
	- Header-only design, no runtime dependencies
- Log routing by level and tag
- Pluggable handlers via function pointers and context
- Optional per-handler tag filters
- Microcontroller-safe 
- Provides synchronous logging today; asynchronous mode is on the roadmap.
- No dynamic allocations (e.g. no STL containers), keeping RAM footprint and CPU overhead extremely low

---

## Index

This project includes a vault-style documentation layout.

- README
- Roadmap
- Examples 
- Documentation -- Doxyfile docs exist, however more Documentation is in the works. 

---

## Project Structure

```
include/
├── Logger.h         # Central logger class and handler registration
├── HandlerEntry.h   # Metadata for registered outputs
├── LogLevel.h       # Log severity enum 
├── LogMessage.h     # Message struct (level, tag, text, timestamp)
├── LogAnywhere.h    # Umbrella include
tests/
├── test Files

build/               # CMake build output
README/              # Obsidian vault for structured docs
```

---

## Example: Serial + File Logger

```cpp
Logger logger;

auto SerialHandler = [](const LogMessage& msg, void* ctx) {
    auto* out = static_cast<std::ostream*>(ctx);
    *out << "[" << toString(msg.level) << "] " << msg.tag << ": " << msg.message << "\n";
};

logger.registerHandler(LogLevel::INFO, SerialHandler, &std::cout);
logger.log(LogLevel::INFO, "SYSTEM", "Startup complete.");
```

See more examples in the docs.

---

---

## Using the Library

**LogAnywhere** is a single-header, dependency-free library.

Just include the umbrella header in your project:

```cpp
#include "LogAnywhere.h"
```

This gives you access to:
- `Logger`
- `LogLevel`
- `LogMessage`
- Handler/filter registration utilities

You can drop the entire `include/` folder into your project if desired.

Project will eventually be posted to platformio's library.
---

## Running Tests (Optional)

Test binaries are provided via CMake + Catch2.

These are not needed, but if you desired to see my testing, here are the steps to build:

### 1. Configure

```bash
cmake -S . -B build -G "MinGW Makefiles"
```

### 2. Build All Tests

```bash
cmake --build build
```

### 3. Run All Tests

```bash
for t in ./build/test_*; do echo "Running $t"; "$t"; done
```

---

## Vault Documentation

The `README/` folder includes developer notes and guides in Obsidian-compatible markdown.

### Option 1: Obsidian
Simply open the LogAnwhere folder in Obsidian.
---

## Project Philosophy

- No assumptions: you control every handler
- No globals: instantiate loggers as needed
- No STL dependency unless you use it in your handlers
- No need to build or install — just include and use
- Logging is a core functionality of embedded, that's their whole goal - this helps centralize that to do what you need.


---

## Roadmap

| Stage             | Description                                    | Status         |
|------------------|------------------------------------------------|----------------|
| Core logger       | Log routing by level and handler registration  | ✅ Complete     |
| Tag filters       | Per-handler tag filtering                      | ✅ Complete     |
| Timestamp hooks   | Custom provider support                        | ✅ Complete     |
| Async support     | Buffer or deferred logging                     | Planned        |
| Built-in backends | Optional serial/MQTT handlers                  | Planned        |
| Log ordering      | Sequential fallback when time is unavailable   | ✅ Complete     |
| Tests             | Full Catch2 coverage                           | In Progress     |
| Docsify Vault     | Markdown documentation via local viewer        | ✅ Available    |

---

## Project Philosophy

- Output is never assumed — it must be explicitly registered
- Logging destinations are function pointers with optional tag filters
- No hardcoded global log state, files, or protocols
- Focused on embedded, bare-metal, and cross-platform compatibility

---

## Vault Documentation

If you cloned the repo with `README/` or `obsidian/`:

### Option 1: Open in Obsidian

- Download Obsidian
- Choose "Open folder as vault"
- Select `README/`

### Option 2: Open `index.html`

- Double-click `README/index.html` for a browser-based viewer (uses Docsify)
- Works offline without build steps

---

## Contribution & Intent

This project is maintained by a single developer for use across embedded and personal tooling projects. It is designed to reflect:

- Real-world constraints of microcontrollers
- A preference for portable, testable, and pluggable software
- An emphasis on design clarity and low-overhead reuse
- Frustration at having to build a log handler for every mesh project. 

If you're reviewing this as part of a hiring - the majority of my code goes to my private gitlab instance for privacy reasons. What's listed here is purely for 
