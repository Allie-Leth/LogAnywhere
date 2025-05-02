# LogAnywhere

*A zero‑allocation, header‑only log router for embedded and cross‑platform C++.*  
Route any log line to any number of **handlers** (serial, file, MQTT, etc.) by **level** and **tag**—without dragging in streams, RTTI or the STL.

---
## Highlights  
- **Header‑only, C++17** – drop `include/` in and go.  
- **Static RAM only** – no `new`, no containers, deterministic footprint.  
- **Pluggable handlers** – register a callback + context pointer.  
- **Tag / level filters** – subscribe handlers to the signals they care about.  
- **Micro‑tuned limits** – change two macros to trade RAM for capacity.  

```cpp
// 6‑handler / 6‑tag build (tiny MCU)
#define LOGANYWHERE_MAX_HANDLERS   6
#define MAX_TAG_SUBSCRIPTIONS      6
#include "LogAnywhere.h"
```

---

## Configuration: Tuning RAM vs. Speed
Adjust the two macros based on your device’s RAM:

```cpp
// LogAnywhereConfig.h
#pragma once
#if defined(ESP32) || defined(CONFIG_IDF_TARGET_ESP32)
// ESP32: ample RAM → larger capacity
  #define LOGANYWHERE_MAX_HANDLERS   16
  #define MAX_TAG_SUBSCRIPTIONS      12
#elif defined(__AVR__) || defined(SMALL_DEVICE)
// Tiny MCU (AVR) → minimal footprint
  #define LOGANYWHERE_MAX_HANDLERS    4
  #define MAX_TAG_SUBSCRIPTIONS       4
#else
// Default desktop or mid-range MCU
  #define LOGANYWHERE_MAX_HANDLERS    8
  #define MAX_TAG_SUBSCRIPTIONS       6
#endif
```

Include this config header **before** any LogAnywhere includes, or define via your build system (`-D` flags).

---

---


### Why LogAnywhere?
LogAnywhere was forged while scaling an ESP32 / Raspberry Pi mesh fleet through CI/CD. Existing log libs collapsed once each node had to stream **simultaneously** to serial, file, MQTT and custom sinks. Re‑implementing handlers every time was painful, so LogAnywhere became the single, header‑only core that *separates routing from output.*

On small devices logs are often mission‑critical telemetry, not just debug noise. Multi‑destination routing is therefore a first‑class feature: pick your sinks, set level/tag filters, and LogAnywhere delivers—completely transport‑agnostic.

## Quick Start
```cpp
#include "LogAnywhere.h"
using namespace LogAnywhere;

static Tag TAG_SYS("SYS");

auto serialOut = [](const LogMessage& m, void*) {
    printf("[%s] %s: %s\n", toString(m.level), m.tag, m.message);
};

HandlerManager mgr;
Logger         log(&mgr);
const Tag* t[] = { &TAG_SYS };

mgr.registerHandlerForTags(LogLevel::INFO, serialOut, nullptr, t, 1);
log.log(LogLevel::INFO, &TAG_SYS, "Boot OK");
```


---
## Building & Tests (optional)
```bash
cmake -S . -B build
cmake --build build && ctest --test-dir build
```

---
## Roadmap (abridged)

| Feature                             | Status |
|:-----------------------------------:|:------:|
| Async / ring-buffer backend         | 🔲     |
| Tuning RAM vs. Speed                | 🔲     |
| Reference handlers (Serial, MQTT)   | 🔲     |
| Dynamic log-level switching         | 🔲     |
| Structured logging (JSON/CBOR)      | 🔲     |
| Async / ring‑buffer backend | 🔲 |
| Reference handlers (Serial, MQTT) | 🔲 |

---
## Contributing
1. Keep it header‑only, zero‑allocation.  
2. Add *Catch2* tests for every new feature.  
3. Target the **`dev`** branch in a PR.

---
© 2025  LogAnywhere – MIT License

