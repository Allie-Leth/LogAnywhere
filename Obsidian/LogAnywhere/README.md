# LogAnywhere

**LogAnywhere** is a lightweight, backend-agnostic logging library for embedded C++ development. It focuses on minimalism, clarity, and total decoupling from output protocols. The result is a flexible log router that works across microcontrollers, Linux targets, and constrained environments.

---

## Why LogAnywhere Exists

Most embedded systems need logs — but in projects involving **meshes and nested mesh topologies**, traditional logging solutions often fall short. When each device may have different capabilities and constraints (some log to serial, others to MQTT, others buffer or forward), a **single, inflexible logging system doesn’t scale**. After rewriting the same scaffolding over and over for different transports, I built **LogAnywhere** to **separate routing logic from output logic**, allowing flexible, pluggable handlers that can be reused across the entire mesh or mesh-of-meshes with minimal overhead. Built for 

You define where messages go. LogAnywhere dispatches based on level, tags, and filters. The library itself never assumes transport or formatting — that's entirely up to the developer.

---

## Key Features

- Embedded design focused - Extremely low overhead
	- Header-only design, no runtime dependencies
- Log routing by level and tag
- Pluggable handlers via function pointers and context
- Optional per-handler tag filters
- Microcontroller-safe 
- Designed for both synchronous and future async backends

---

## Index

This project includes a vault-style documentation layout. Suggested pages:

- Overview – core philosophy, quick start
- Getting Started – minimal setup
- Handlers – writing and registering outputs
- Tag Filtering – focused routing
- Timestamps – default + custom time providers
- Testing – structure and examples using Catch2
- Design Notes – architecture and rationale
- Roadmap – current and planned features

Use the provided `index.html` viewer (Docsify) or open the vault directly in Obsidian.

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
├── test_Logger.cpp
├── test_LogMessage.cpp
├── test_Handlers.cpp
├── test_Timestamps.cpp
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

---

## Running Tests (Optional)

Test binaries are provided via CMake + Catch2.

Only needed if you're contributing or modifying internals.

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

### Option 1: Open in Obsidian

- Open Obsidian → “Open Folder as Vault” → select `README/`

### Option 2: Open in Browser

- Use `README/index.html` to view docs in any web browser (Docsify)

---

## Project Philosophy

- No assumptions: you control every handler
- No globals: instantiate loggers as needed
- No STL dependency unless you use it in your handlers
- No need to build or install — just include and use


---

## Roadmap

| Stage             | Description                                    | Status         |
|------------------|------------------------------------------------|----------------|
| Core logger       | Log routing by level and handler registration  | ✅ Complete     |
| Tag filters       | Per-handler tag filtering                      | ✅ Complete     |
| Timestamp hooks   | Custom provider support                        | ✅ Complete     |
| Async support     | Buffer or deferred logging                     | Planned        |
| Built-in backends | Optional serial/MQTT handlers                  | Planned        |
| Log ordering      | Monotonic fallback when time is unavailable    | ✅ Complete     |
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

If you're reviewing this as part of a hiring, demo, or collaboration context, feel free to explore the examples, test coverage, and documentation vault for a deeper understanding of the approach and architecture.
