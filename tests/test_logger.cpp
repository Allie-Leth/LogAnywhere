#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../include/LogAnywhere.h"

using namespace LogAnywhere;

static std::string lastOutput;

void TestHandler(const LogMessage& msg, void* ctx) {
    lastOutput = std::string("[") + toString(msg.level) + "] " + msg.tag + ": " + msg.message;
}

TEST_CASE("Logger dispatches messages to registered handlers", "[Logger]") {
    HandlerManager manager;
    Logger logger(&manager);

    std::string lastOutput;
    auto TestHandler = [](const LogMessage& msg, void* ctx) {
        *static_cast<std::string*>(ctx) = "[" + std::string(toString(msg.level)) + "] " + msg.tag + ": " + msg.message;
    };

    manager.registerHandler(LogLevel::INFO, TestHandler, &lastOutput, nullptr, "DispatchTest");

    logger.log(LogLevel::INFO, "CORE", "System started");

    REQUIRE_FALSE(lastOutput.empty());
    REQUIRE(lastOutput.find("System started") != std::string::npos);
}


TEST_CASE("Logger filters by minimum log level", "[Logger][Filter][Level]") {
    HandlerManager manager;
    Logger logger(&manager);

    std::string lastOutput;
    auto TestHandler = [](const LogMessage& msg, void* ctx) {
        *static_cast<std::string*>(ctx) = "[" + std::string(toString(msg.level)) + "] " + msg.tag + ": " + msg.message;
    };

    manager.registerHandler(LogLevel::ERR, TestHandler, &lastOutput, nullptr, "LevelFilterTest");

    logger.log(LogLevel::INFO, "CORE", "Ignored message");

    REQUIRE(lastOutput.empty()); // Should not log
}


TEST_CASE("Logger supports tag filtering during dispatch", "[Logger][Filter][Tag]") {
    HandlerManager manager;
    Logger logger(&manager);

    std::string lastOutput;
    auto TestHandler = [](const LogMessage& msg, void* ctx) {
        *static_cast<std::string*>(ctx) = "[" + std::string(toString(msg.level)) + "] " + msg.tag + ": " + msg.message;
    };

    auto onlyOTA = [](const char* tag, void*) -> bool {
        return strcmp(tag, "OTA") == 0;
    };

    manager.registerHandler(LogLevel::INFO, TestHandler, &lastOutput, onlyOTA, "TagFilterTest");

    logger.log(LogLevel::INFO, "BOOT", "Not OTA");
    REQUIRE(lastOutput.empty());

    logger.log(LogLevel::INFO, "OTA", "Firmware update started");
    REQUIRE(lastOutput.find("Firmware update started") != std::string::npos);
}
