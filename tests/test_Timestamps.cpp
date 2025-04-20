#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <thread>
#include <chrono>

#include "../include/Logger.h"

using namespace LogAnywhere;

TEST_CASE("Logger uses default timestamp (zero) when none provided", "[Logger][Timestamp]") {
    Logger logger;
    uint64_t captured = 42;

    auto CapturingHandler = [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    };

    logger.registerHandler(LogLevel::INFO, CapturingHandler, &captured);
    logger.log(LogLevel::INFO, "TS_TEST", "Default timestamp");

    REQUIRE(captured == 0);
}

TEST_CASE("Logger uses custom timestamp provider", "[Logger][Timestamp]") {
    Logger logger;
    uint64_t captured = 0;

    auto CustomTime = []() -> uint64_t {
        return 123456789;
    };

    logger.setTimestampProvider(CustomTime);

    auto CapturingHandler = [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    };

    logger.registerHandler(LogLevel::INFO, CapturingHandler, &captured);
    logger.log(LogLevel::INFO, "TS_TEST", "Should use custom time");

    REQUIRE(captured == 123456789);
}

TEST_CASE("Logger prefers explicit timestamp over provider", "[Logger][Timestamp]") {
    Logger logger;
    uint64_t captured = 0;

    auto WrongTime = []() -> uint64_t {
        return 555;
    };

    logger.setTimestampProvider(WrongTime);

    auto CapturingHandler = [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    };

    logger.registerHandler(LogLevel::INFO, CapturingHandler, &captured);
    logger.log(LogLevel::INFO, "TS_TEST", "Should use explicit", 987654321);

    REQUIRE(captured == 987654321);
}


TEST_CASE("Logger uses custom NTP-like timestamp provider and updates between calls", "[Logger][Timestamp][NTP]") {
    Logger logger;
    uint64_t captured1 = 0;
    uint64_t captured2 = 0;

    // Simulated NTP provider using system clock
    auto NtpLikeTime = []() -> uint64_t {
        using namespace std::chrono;
        auto now = system_clock::now().time_since_epoch();
        return duration_cast<microseconds>(now).count();
    };

    logger.setTimestampProvider(NtpLikeTime);

    // Capture 1st timestamp
    logger.registerHandler(LogLevel::INFO, [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    }, &captured1);

    logger.log(LogLevel::INFO, "NTP_TEST", "First log");

    // Wait a bit (~50ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Capture 2nd timestamp using a different context pointer
    logger.registerHandler(LogLevel::INFO, [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    }, &captured2);

    logger.log(LogLevel::INFO, "NTP_TEST", "Second log");

    REQUIRE(captured1 != 0);
    REQUIRE(captured2 != 0);
    REQUIRE(captured2 > captured1);  // ensure it's increasing
}
