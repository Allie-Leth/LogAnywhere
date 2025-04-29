#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../include/LogAnywhere.h"
#include "../include/Tag.h"
#include <string>

using namespace LogAnywhere;

TEST_CASE("Logger dispatches messages to registered handlers", "[Logger]") {
    // fresh tags for this test
    Tag CORE("CORE");
    Tag BOOT("BOOT");
    Tag OTA ("OTA");

    HandlerManager manager;
    Logger        logger(&manager);

    std::string lastOutput;
    auto TestHandler = [](const LogMessage& msg, void* ctx) {
        *static_cast<std::string*>(ctx) =
            "[" + std::string(toString(msg.level)) + "] "
            + msg.tag + ": " + msg.message;
    };

    // Subscribe only to CORE
    const Tag* tags[] = { &CORE };
    REQUIRE(manager.registerHandlerForTags(
        LogLevel::INFO,
        TestHandler,
        &lastOutput,
        tags,
        1,
        "DispatchTest"
    ));

    // Dispatch via Tag* → only CORE is subscribed
    logger.log(LogLevel::INFO, &CORE, "System started");

    REQUIRE_FALSE(lastOutput.empty());
    REQUIRE(lastOutput.find("System started") != std::string::npos);
}

TEST_CASE("Logger filters by minimum log level", "[Logger][Filter][Level]") {
    // fresh tags for this test
    Tag CORE("CORE");
    Tag BOOT("BOOT");
    Tag OTA ("OTA");

    HandlerManager manager;
    Logger        logger(&manager);

    std::string lastOutput;
    auto TestHandler = [](const LogMessage& msg, void* ctx) {
        *static_cast<std::string*>(ctx) =
            "[" + std::string(toString(msg.level)) + "] "
            + msg.tag + ": " + msg.message;
    };

    // Subscribe to CORE but with ERR threshold
    const Tag* tags[] = { &CORE };
    REQUIRE(manager.registerHandlerForTags(
        LogLevel::ERR,
        TestHandler,
        &lastOutput,
        tags,
        1,
        "LevelFilterTest"
    ));

    // Log below threshold → should not invoke handler
    logger.log(LogLevel::INFO, &CORE, "Ignored message");
    REQUIRE(lastOutput.empty());
}

TEST_CASE("HandlerManager::registerHandlerForTags returns true for a valid subscription",
          "[HandlerManager][Register]")
{
    // fresh tags for this test
    Tag CORE("CORE");
    Tag BOOT("BOOT");
    Tag OTA ("OTA");

    HandlerManager mgr;
    std::string ctx;
    auto noOp = [](const LogMessage&, void*){};
    const Tag* tags[] = { &OTA };

    REQUIRE(mgr.registerHandlerForTags(
        LogLevel::INFO,
        noOp,
        &ctx,
        tags,
        1,
        "RegisterTest"
    ));
}

TEST_CASE("Logger does NOT invoke handler when logging to an unsubscribed tag",
          "[Logger][Filter][Negative]")
{
    // fresh tags for this test
    Tag CORE("CORE");
    Tag BOOT("BOOT");
    Tag OTA ("OTA");

    HandlerManager mgr;
    Logger        logger(&mgr);

    std::string lastOutput;
    auto TestHandler = [](const LogMessage& msg, void* ctx) {
        *static_cast<std::string*>(ctx) = msg.message;
    };

    // subscribe only to OTA
    const Tag* tags[] = { &OTA };
    REQUIRE(mgr.registerHandlerForTags(
        LogLevel::INFO,
        TestHandler,
        &lastOutput,
        tags,
        1,
        "NoDispatchTest"
    ));

    // log to BOOT → should not touch lastOutput
    logger.log(LogLevel::INFO, &BOOT, "Boot message");
    REQUIRE(lastOutput.empty());
}

TEST_CASE("Logger invokes handler when logging to a subscribed tag",
          "[Logger][Filter][Positive]")
{
    // fresh tags for this test
    Tag CORE("CORE");
    Tag BOOT("BOOT");
    Tag OTA ("OTA");

    HandlerManager mgr;
    Logger        logger(&mgr);

    std::string lastOutput;
    auto TestHandler = [](const LogMessage& msg, void* ctx) {
        *static_cast<std::string*>(ctx) =
            std::string("[") + toString(msg.level) + "] "
            + msg.tag + ": " + msg.message;
    };

    // subscribe only to OTA
    const Tag* tags[] = { &OTA };
    REQUIRE(mgr.registerHandlerForTags(
        LogLevel::INFO,
        TestHandler,
        &lastOutput,
        tags,
        1,
        "PositiveTest"
    ));

    // log to OTA → should fill lastOutput
    logger.log(LogLevel::INFO, &OTA, "Firmware update started");
    REQUIRE(lastOutput.find("Firmware update started") != std::string::npos);
}
