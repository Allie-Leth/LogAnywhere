# LogAnywhere

**LogAnywhere** is a minimal, flexible, and backend-agnostic logging library designed for embedded systems and constrained C++ environments.

It allows you to define **custom log handlers** and **register them by level**, while remaining completely decoupled from any specific output transport (e.g., serial, file, MQTT).

## Why LogAnywhere Exists

This library was created out of being irritated at code duplication across projects: I kept having to rewrite ad-hoc logging systems for each embedded project — whether it was logging over serial, buffering to memory, or forwarding to MQTT. Most projects had slightly different needs, but the core pattern was always the same.

LogAnywhere solves this by abstracting the *routing* of log messages from the *output method*. It lets you register handlers with your own logic and lets the logger focus purely on dispatching. No assumptions, no baked-in transports — just flexible, portable logging for constrained systems.


>  Output goes *only* to handlers you register — LogAnywhere never assumes where to log.

---

##  Features

- Log routing by level and tag
- Pluggable log handlers (function pointers + context)
- Optional tag filtering per handler for context
- No external dependencies (pure C++)
- Tiny footprint — no STL required if avoided in user handlers
- Easily adaptable to async, MQTT, filesystem, buffers, etc.
- Designed to be easily testable and embedded-safe

---

##  Project Structure

```
include/
├── Logger.h         # Core class for handler registration and log dispatch
├── LogLevel.h       # Enum for standard log levels
├── LogMessage.h     # Struct for messages
├── LogAnywhere.h    # Umbrella include for the entire API
tests/
├── test_Logger.cpp
├── test_LogMessage.cpp
├── test_Handlers.cpp
build/                # CMake-generated binaries
```

---

##  Getting Started

### 1. Clone the project

```bash
git clone https://your.gitlab.instance/youruser/loganywhere.git
cd loganywhere
```

### 2. Build with CMake

```bash
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

---

## Running Tests

All test files build into separate binaries.

### Run them all:

```bash
for t in ./build/test_*; do echo "🔹 Running $t"; "$t"; done
```

Or run individual tests:

```bash
./build/test_Logger
./build/test_Handlers
```

---

## Adding a Log Handler

```cpp
Logger logger;

auto SerialHandler = [](const LogMessage& msg, void* ctx) {
    auto* stream = static_cast<std::ostream*>(ctx);
    *stream << "[" << toString(msg.level) << "] " << msg.tag << ": " << msg.message << "\n";
};

logger.registerHandler(LogLevel::INFO, SerialHandler, &std::cout);
logger.log(LogLevel::INFO, "CORE", "Started system!");
```

---

## Roadmap

| Stage             | Description                                    | Status        |
|-------------------|------------------------------------------------|---------------|
| Core logger       | Log routing by level and handler registration  | ✅ Complete   |
| Tag filters       | Per-handler tag filtering                      | ✅ Complete   |
| Async support     | Buffer or queue-based deferred log delivery    | 🔜 Planned    |
| Built-in backends | Optional modules for Serial, File, MQTT        | 🔜 Planned    |
| Tests             | Catch2 test coverage for all modules           | ✅ In Progress|
| Doxygen Docs      | Generate API diagrams and usage docs           | ✅ Available  |

---

## Project Philosophy

- Small and dependency-free
- All output is explicitly registered (not assumed)
- All output formats are pluggable
- No hard-coded paths, files, or protocols
- Designed for microcontrollers and embedded apps

---

## Contribution

This is a **solo developer** project intended for internal use and custom integration. If I find this project particularly useful going forward I will move it to github for public use, but as of now I see it as too niche to publish. 

If you're reading this for hiring or demo purposes:
- The focus is on clarity, maintainability, and minimalism.
- The tests and design reflect embedded and production-grade patterns.



---

## Vault + Documentation

This repo may include an `obsidian/` or `README/` vault if cloned with full documentation.

To open the vault:
- Use [Obsidian](https://obsidian.md/)
- Or use the included `index.html` (Docsify-based viewer)

---
