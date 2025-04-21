#  LogAnywhere Project Roadmap

LogAnywhere is a log-agnostic routing layer for embedded and systems-level development. All log destinations (serial, BLE, MQTT, etc.) are built on top of a shared dispatch core with the following priorities:

---

##  Platform-Agnostic Design Constraints

To maximize reusability, the core LogAnywhere implementation avoids:
- Any dependency on `<Arduino.h>`
- Use of `millis()`, `Serial.print`, or other platform-specific APIs
- Use of dynamic memory or `std::string` in required paths

### ✅ Instead:
- Only C++11 features and standard headers are used
- Log routing is handled entirely through registered handlers
- Output behavior is fully delegated via `void(LogMessage&, void*)`
- Optional formatters or transports may add platform-specific extensions, but they are not part of the core

This ensures compatibility with:
- Arduino / ESP32
- Linux CLI tools
- Bare metal microcontrollers
- RTOS-based systems (e.g., FreeRTOS)

---

# Roadmap
---

## High Priority - Cannot be published until finished
- [ ] **Implement Error handling in all parts of code**
- [ ] **Fail-safe behavior for unregistered or invalid handlers**
- [ ] Finish test cases
- [ ] Provide more fine-grained control of logging
- [ ] **Support for deregistering handlers**
- [ ] Dynamic handler control
- [ ] **Boundary enforcement on log message content**
- [ ] **Async logging**
	- [ ] Thread safety warnings
- [ ] Default safe log fallback
- [ ] Time handling edge cases
- [ ] Implement Logf
##   — Metadata Injection & Safety

Enhance logs without tying the logger to any platform.

- [ ] Support external timestamp injection via function pointer:
  - `void setTimestampSource(std::function<uint64_t()> fn)`
  - This replaces `millis()` entirely
- [ ] Add optional source tags (filename / function) via macro helpers
- [ ] Decide between `std::string` vs `const char*` as base API (or overload both)
- [x] Create log-level filtering mechanism
- [x] Handler-level filtering (`only WARN and ERROR`, etc.)

---

##  Generic Backends (Built on Core)

Once the core is fully stable, build transport backends on top.

- [ ] Serial backend
- [ ] MQTT backend (takes context pointer to MQTT client)
- [ ] SPIFFS file writer
- [ ] Queue/deferred handler (async buffer interface)
- [ ] BLE log broadcaster
- [ ] JSON formatter layer (optional pre-handler step)

---

##  Embedded Safety & Crash Resilience

Make it hardened for long uptime and embedded environments.

- [ ] Thread-safe log dispatch
- [ ] Compile-time log-level stripping
- [ ] Ring buffer with crash flag sync (for delayed upload)
- [ ] Boot-persistent session ID injection

---

## Notes

- `millis()` rollover (~49 days) is **not** used. Timestamps must be injected via platform-agnostic hooks. 
- `log()` will **never** handle output directly. All output must be registered.
- All log features must be usable **with no heap allocation** if desired.

This document lives in `vault/Roadmap.md` and is exported for public consumption.
