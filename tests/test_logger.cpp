#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../include/LogAnywhere.h"

using namespace LogAnywhere;

static std::string lastOutput;

void TestHandler(const LogMessage& msg, void* ctx) {
    lastOutput = std::string("[") + toString(msg.level) + "] " + msg.tag + ": " + msg.message;
}

TEST_CASE("Logger dispatches messages to registered handlers", "[Logger][BasicDispatch]") 
{
    lastOutput.clear();

    REQUIRE(registerHandler(LogLevel::INFO, TestHandler, nullptr, nullptr, "BasicDispatchHandler"));

    log(LogLevel::INFO, "CORE", "System started");

    REQUIRE(!lastOutput.empty());
    REQUIRE(lastOutput.find("System started") != std::string::npos);
}

TEST_CASE("Logger filters by minimum log level", "[Logger][Filter][Level]") 
{
    lastOutput.clear();

    REQUIRE(registerHandler(LogLevel::ERR, TestHandler, nullptr, nullptr, "LevelFilterHandler"));

    log(LogLevel::INFO, "CORE", "Ignored message");

    REQUIRE(lastOutput.empty()); // INFO < ERR, so should not log
}

TEST_CASE("Logger supports tag filtering during dispatch", "[Logger][Filter][Tag]") 
{
    lastOutput.clear();

    auto onlyOTA = [](const char* tag, void*) -> bool {
        return strcmp(tag, "OTA") == 0;
    };

    REQUIRE(registerHandler(LogLevel::INFO, TestHandler, nullptr, onlyOTA, "TagFilterHandler"));

    log(LogLevel::INFO, "BOOT", "Not OTA"); // Should be filtered out
    REQUIRE(lastOutput.empty());

    log(LogLevel::INFO, "OTA", "Firmware update started"); // Should pass
    REQUIRE(lastOutput.find("Firmware update started") != std::string::npos);
}
