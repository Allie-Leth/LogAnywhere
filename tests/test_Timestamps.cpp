#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <thread>
#include <chrono>
#include "../include/LogAnywhere.h"
#include "../include/Tag.h"

using namespace LogAnywhere;

// Tags used in these tests
static Tag TAG_SEQ_TEST("SEQ_TEST");
static Tag TAG_TS_TEST ("TS_TEST");
static Tag TAG_TS1    ("TS1");
static Tag TAG_TS2    ("TS2");

TEST_CASE("Logger uses sequential default timestamps when none provided", "[Logger][Timestamp][Sequence]") 
{
    uint64_t capturedFirst  = 0;
    uint64_t capturedSecond = 0;

    auto capturingHandlerFirst = [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    };
    auto capturingHandlerSecond = [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    };

    // Subscribe first handler to SEQ_TEST
    const Tag* seqTags[] = { &TAG_SEQ_TEST };
    REQUIRE(registerHandler(
        LogLevel::INFO,
        capturingHandlerFirst,
        &capturedFirst,
        seqTags,
        1,
        "FirstSequenceCapture"
    ));

    // Emit first log
    log(LogLevel::INFO, &TAG_SEQ_TEST, "First sequence log");

    // Remove first handler
    REQUIRE(deleteHandlerByName("FirstSequenceCapture"));

    // Subscribe second handler to the same tag
    REQUIRE(registerHandler(
        LogLevel::INFO,
        capturingHandlerSecond,
        &capturedSecond,
        seqTags,
        1,
        "SecondSequenceCapture"
    ));

    // Emit second log
    log(LogLevel::INFO, &TAG_SEQ_TEST, "Second sequence log");

    // Verify timestamps
    REQUIRE(capturedFirst >= 1);                            // Sequence starts ≥1
    REQUIRE(capturedSecond == capturedFirst + 1);           // Exactly increments by 1
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

    // Subscribe to TS_TEST
    const Tag* tsTags[] = { &TAG_TS_TEST };
    REQUIRE(registerHandler(
        LogLevel::INFO,
        capturingHandler,
        &captured,
        tsTags,
        1,
        "CustomTimestampTest"
    ));

    // Emit log (uses customTime)
    log(LogLevel::INFO, &TAG_TS_TEST, "Should use custom time");

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

    // Subscribe to TS_TEST
    const Tag* tsTags[] = { &TAG_TS_TEST };
    REQUIRE(registerHandler(
        LogLevel::INFO,
        capturingHandler,
        &captured,
        tsTags,
        1,
        "ExplicitTimestampTest"
    ));

    // Emit log with explicit timestamp
    log(LogLevel::INFO, &TAG_TS_TEST, "Should use explicit", 987654321);

    REQUIRE(captured == 987654321);
}

TEST_CASE("Logger uses custom timestamp provider and updates between logs", "[Logger][Timestamp][NTP]") 
{
    uint64_t captured1 = 0;
    uint64_t captured2 = 0;

    auto ntpLikeTime = []() -> uint64_t {
        using namespace std::chrono;
        auto now = system_clock::now().time_since_epoch();
        return duration_cast<microseconds>(now).count();
    };
    logger.setTimestampProvider(ntpLikeTime);

    auto handler1 = [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    };
    auto handler2 = [](const LogMessage& msg, void* ctx) {
        *static_cast<uint64_t*>(ctx) = msg.timestamp;
    };

    // Subscribe handler1 to TS1
    const Tag* tags1[] = { &TAG_TS1 };
    REQUIRE(registerHandler(
        LogLevel::INFO,
        handler1,
        &captured1,
        tags1,
        1,
        "TimestampCapture1"
    ));
    log(LogLevel::INFO, &TAG_TS1, "First timestamped log");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Subscribe handler2 to TS2
    const Tag* tags2[] = { &TAG_TS2 };
    REQUIRE(registerHandler(
        LogLevel::INFO,
        handler2,
        &captured2,
        tags2,
        1,
        "TimestampCapture2"
    ));
    log(LogLevel::INFO, &TAG_TS2, "Second timestamped log");

    // Validate real-world NTP-like timestamps
    REQUIRE(captured1 > 1'500'000'000'000'000); // Reasonable lower bound
    REQUIRE(captured2 > captured1);             // Timestamp monotonicity
}
