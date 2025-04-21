#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <thread>
#include <chrono>

#include "../include/Logger.h"

using namespace LogAnywhere;

TEST_CASE("Logger uses default timestamp (zero) when none provided", "[Logger][Timestamp]")
{
    Logger logger;
    uint64_t captured = 42;

    auto CapturingHandler = [](const LogMessage &msg, void *ctx)
    {
        *static_cast<uint64_t *>(ctx) = msg.timestamp;
    };

    logger.registerHandler(LogLevel::INFO, CapturingHandler, &captured);
    logger.log(LogLevel::INFO, "TS_TEST", "Default timestamp");

    REQUIRE(captured == 1);
}

TEST_CASE("Logger uses custom timestamp provider", "[Logger][Timestamp]")
{
    Logger logger;
    uint64_t captured = 0;

    auto CustomTime = []() -> uint64_t
    {
        return 123456789;
    };

    logger.setTimestampProvider(CustomTime);

    auto CapturingHandler = [](const LogMessage &msg, void *ctx)
    {
        *static_cast<uint64_t *>(ctx) = msg.timestamp;
    };

    logger.registerHandler(LogLevel::INFO, CapturingHandler, &captured);
    logger.log(LogLevel::INFO, "TS_TEST", "Should use custom time");

    REQUIRE(captured == 123456789);
}

TEST_CASE("Logger prefers explicit timestamp over provider", "[Logger][Timestamp]")
{
    Logger logger;
    uint64_t captured = 0;

    auto WrongTime = []() -> uint64_t
    {
        return 555;
    };

    logger.setTimestampProvider(WrongTime);

    auto CapturingHandler = [](const LogMessage &msg, void *ctx)
    {
        *static_cast<uint64_t *>(ctx) = msg.timestamp;
    };

    logger.registerHandler(LogLevel::INFO, CapturingHandler, &captured);
    logger.log(LogLevel::INFO, "TS_TEST", "Should use explicit", 987654321);

    REQUIRE(captured == 987654321);
}

TEST_CASE("Logger uses custom timestamp provider and updates between calls", "[Logger][Timestamp][NTP]")
{
    Logger logger;
    uint64_t captured1 = 0;
    uint64_t captured2 = 0;

    auto NtpLikeTime = []() -> uint64_t
    {
        using namespace std::chrono;
        auto now = system_clock::now().time_since_epoch();
        auto us = duration_cast<microseconds>(now).count();
        std::cout << "system_clock: " << us << "\n";
        return us;
    };

    logger.setTimestampProvider(NtpLikeTime);

    // First handler captures into captured1
    auto handler1 = [](const LogMessage &msg, void *ctx)
    {
        *static_cast<uint64_t *>(ctx) = msg.timestamp;
    };

    // Second handler captures into captured2
    auto handler2 = [](const LogMessage &msg, void *ctx)
    {
        *static_cast<uint64_t *>(ctx) = msg.timestamp;
    };
    auto filter1 = [](const char *tag, void *)
    { return strcmp(tag, "TS1") == 0; };
    auto filter2 = [](const char *tag, void *)
    { return strcmp(tag, "TS2") == 0; };

    logger.registerHandlerFiltered(LogLevel::INFO, handler1, &captured1, filter1);
    logger.log(LogLevel::INFO, "TS1", "First log");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    logger.registerHandlerFiltered(LogLevel::INFO, handler2, &captured2, filter2);
    logger.log(LogLevel::INFO, "TS2", "Second log");

    REQUIRE(captured1 > 1'500'000'000'000'000); // Around 2017+
    REQUIRE(captured2 > captured1);
}
