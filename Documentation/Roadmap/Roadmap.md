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

## v1.0 (Released)
- ✅ Core routing by level + tag

---
## v1.1 (Released)
- ✅ Tag-based dispatching 
    - This required a full code refactor so other features were pushed back. This laid the groundwork for my more dynamic handling - 

---
## v1.2 - Quality of Life Improvements (In-progress)
- 🔲 Boilerplate refinement and QoL improvements
    - Currently using this library requires more boilerplate than I find acceptable, registering should be simpler. 

---
## v1.3 - Memory & Speed refinements (In progress)
- 🔲Currently LogAnwhere requires trade offs of RAM usage VS speed. This is suboptimal and does not fit the design goals for embedded systems. This will result in a refactor of how it handles clients and tags on the backend. 
- 🔲 Async / ring-buffer backend
---
## v2.0+ (Future / Backlog)
- 🔲 Built-in reference handlers (Serial, MQTT)
- 🔲 Structured log handling
- 🔲 Dynamic log-level switching
- 🔲 Thread-safe dispatch & compile-time stripping
- 🔲 Extended metadata injection (source file, function)
- 🔲 Crash-resilient ring buffer with persistent IDs
- 🔲 BLE log broadcaster

---
© 2025 LogAnywhere – MIT License

