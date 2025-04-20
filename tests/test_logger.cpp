#define CATCH_CONFIG_MAIN
#include "catch.hpp"


#include "../include/Logger.h"
#include "../include/LogLevel.h"
#include "../include/LogMessage.h"

using namespace LogAnywhere;

static std::string lastOutput;
void TestHandler(const LogMessage& msg, void* ctx) {
    lastOutput = std::string("[") + toString(msg.level) + "] " + msg.tag + ": " + msg.message;
}

TEST_CASE("Logger dispatches messages to registered handlers", "[logger]") {
    Logger logger;
    lastOutput.clear();

    logger.registerHandler(LogLevel::INFO, TestHandler);
    logger.log(LogLevel::INFO, "CORE", "System started");

    REQUIRE(!lastOutput.empty());
    REQUIRE(lastOutput.find("System started") != std::string::npos);
}

TEST_CASE("Logger filters by log level", "[logger]") {
    Logger logger;
    lastOutput.clear();

    logger.registerHandler(LogLevel::ERR, TestHandler);
    logger.log(LogLevel::INFO, "CORE", "Ignored message");

    REQUIRE(lastOutput.empty());
}

TEST_CASE("Logger supports tag filtering", "[logger]") {
    Logger logger;
    lastOutput.clear();

    auto onlyOTA = [](const char* tag, void*) -> bool {
        return strcmp(tag, "OTA") == 0;
    };

    logger.registerHandlerFiltered(LogLevel::INFO, TestHandler, nullptr, onlyOTA);
    logger.log(LogLevel::INFO, "BOOT", "Not OTA");
    REQUIRE(lastOutput.empty());

    logger.log(LogLevel::INFO, "OTA", "Firmware update started");
    REQUIRE(lastOutput.find("Firmware update started") != std::string::npos);
}
