#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <thread>
#include <chrono>
#include "../include/LogAnywhere.h"

using namespace LogAnywhere;

TEST_CASE("Logger uses sequential default timestamps when none provided", "[Logger][Timestamp][Sequence]") 
{
    uint64_t capturedFirst = 0;
    uint64_t capturedSecond = 0;

    auto capturingHandlerFirst = [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    };

    auto capturingHandlerSecond = [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    };

    // First handler captures first timestamp
    REQUIRE(registerHandler(LogLevel::INFO, capturingHandlerFirst, &capturedFirst, nullptr, "FirstSequenceCapture"));

    log(LogLevel::INFO, "SEQ_TEST", "First sequence log");

    // Unregister the first handler to avoid double capturing
    REQUIRE(unregisterHandlerByName("FirstSequenceCapture"));

    // Second handler captures second timestamp
    REQUIRE(registerHandler(LogLevel::INFO, capturingHandlerSecond, &capturedSecond, nullptr, "SecondSequenceCapture"));

    log(LogLevel::INFO, "SEQ_TEST", "Second sequence log");

    // Test expectations:
    REQUIRE(capturedFirst >= 1);        // Should start at 1 or higher
    REQUIRE(capturedSecond == capturedFirst + 1);  // Should increment by exactly 1
}


TEST_CASE("Logger uses custom timestamp provider", "[Logger][Timestamp]") 
{
    uint64_t captured = 0;

    auto customTime = []() -> uint64_t {
        return 123456789;
    };

    logger.setTimestampProvider(customTime);

    auto capturingHandler = [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    };

    REQUIRE(registerHandler(LogLevel::INFO, capturingHandler, &captured, nullptr, "CustomTimestampTest"));

    log(LogLevel::INFO, "TS_TEST", "Should use custom time");

    REQUIRE(captured == 123456789);
}

TEST_CASE("Logger prefers explicit timestamp over provider", "[Logger][Timestamp]") 
{
    uint64_t captured = 0;

    auto wrongTime = []() -> uint64_t {
        return 555;
    };

    logger.setTimestampProvider(wrongTime);

    auto capturingHandler = [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    };

    REQUIRE(registerHandler(LogLevel::INFO, capturingHandler, &captured, nullptr, "ExplicitTimestampTest"));

    log(LogLevel::INFO, "TS_TEST", "Should use explicit", 987654321);

    REQUIRE(captured == 987654321);
}

TEST_CASE("Logger uses custom timestamp provider and updates between logs", "[Logger][Timestamp][NTP]") 
{
    uint64_t captured1 = 0;
    uint64_t captured2 = 0;

    auto ntpLikeTime = []() -> uint64_t {
        using namespace std::chrono;
        auto now = system_clock::now().time_since_epoch();
        auto us = duration_cast<microseconds>(now).count();
        return us;
    };

    logger.setTimestampProvider(ntpLikeTime);

    auto handler1 = [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    };

    auto handler2 = [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    };

    auto filter1 = [](const char* tag, void*) -> bool {
        return strcmp(tag, "TS1") == 0;
    };

    auto filter2 = [](const char* tag, void*) -> bool {
        return strcmp(tag, "TS2") == 0;
    };

    REQUIRE(registerHandler(LogLevel::INFO, handler1, &captured1, filter1, "TimestampCapture1"));
    log(LogLevel::INFO, "TS1", "First timestamped log");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    REQUIRE(registerHandler(LogLevel::INFO, handler2, &captured2, filter2, "TimestampCapture2"));
    log(LogLevel::INFO, "TS2", "Second timestamped log");

    REQUIRE(captured1 > 1'500'000'000'000'000); // Should be a real-world timestamp (past ~2017)
    REQUIRE(captured2 > captured1); // Captured2 should happen after Captured1
}
