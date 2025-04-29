# LogAnywhere Project Roadmap

_LogAnywhere is a log-agnostic routing layer for embedded and systems-level C++._  
All log destinations (serial, BLE, MQTT, etc.) build on a shared dispatch core with strict design constraints:

---
## Platform-Agnostic Design Constraints

- Only C++11 standard headers & features
- Handler-driven routing (`void(const LogMessage&, void*)`)
- Optional transport/formatter extensions sit out‑of‑core

Compatibility targets:
- Arduino / ESP32  
- Linux CLI tools  
- Bare‑metal MCUs  
- RTOS systems (FreeRTOS, Zephyr)

---
# Roadmap

## v1.0 (Complete)
- ✅ Core routing by level + tag

---
## v1.1 (In Progress)
- ✅ Tag-based dispatching
- 🔲 Tuning RAM vs. Speed configurations

---
## v1.2 (Next Up)
- 🔲 Async / ring-buffer backend
- 🔲 Built-in reference handlers (Serial, MQTT)
- 🔲 Structured-log output (JSON / CBOR layer)

---
## v2.0+ (Future / Backlog)
- 🔲 Dynamic log-level switching
- 🔲 Thread-safe dispatch & compile-time stripping
- 🔲 Extended metadata injection (source file, function)
- 🔲 Crash-resilient ring buffer with persistent IDs
- 🔲 BLE log broadcaster

---
© 2025 LogAnywhere – MIT License

