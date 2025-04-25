# LogAnywhere

**LogAnywhere** is a lightweight, backend-agnostic logging library for embedded C++ development. It focuses on minimalism, clarity, and total decoupling from output protocols. The result is a flexible log router that works across microcontrollers, Linux targets, and constrained environments. 

---

## Why LogAnywhere Exists

Most embedded systems need logs — but in projects involving **meshes and nested mesh topologies**, I've found that traditional logging strategies fall short from what I needed. When each device may have different capabilities and constraints - and importantly may need to point to multiple outputs like serial, text file, mqtt, tcp/ip, etc - so a **single, inflexible logging system doesn’t scale**. After rewriting the same scaffolding over and over for different transports, I built **LogAnywhere** to **separate routing logic from output logic**, allowing flexible, pluggable handlers that can be reused across the entire mesh or mesh-of-meshes with minimal overhead. Built for embedded, usable anywhere. 

You define where messages go. LogAnywhere dispatches based on level, tags, and filters. The library itself never assumes transport or formatting — that's entirely up to the developer.

---

## Key Features

- Embedded design focused - Extremely low overhead
	- Header-only design, no runtime dependencies
- Log routing by level and tag
- Pluggable handlers via function pointers and context
- Optional per-handler tag filters
- Microcontroller-safe 
- Designed for both synchronous and future async backends.

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
