#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../include/LogAnywhere.h"
#include "../include/Tag.h"

using namespace LogAnywhere;

// A simple handler that counts the number of times it is invoked.
static void countingHandler(const LogAnywhere::LogMessage &msg, void *ctx)
{
    auto *count = static_cast<int *>(ctx);
    ++*count;
}

// Verifies that registerHandlerForTags returns true when adding a valid subscription
TEST_CASE("HandlerManager::registerHandlerForTags returns true for valid subscription",
          "[HandlerManager][registerHandlerForTags]")
{
    // Create fresh Tags and Manager for isolation
    Tag CORE("CORE"), BOOT("BOOT"), OTA("OTA");
    HandlerManager mgr;
    std::string ctx;
    auto noOp = [](const LogMessage &, void *) {};
    const Tag *tags[] = {&OTA};

    // Under capacity, registration must succeed
    REQUIRE(mgr.registerHandlerForTags(
        LogLevel::INFO,
        noOp,
        &ctx,
        tags,
        1,
        "RegisterTest"));
}

// Verifies the mapping of LogLevel enum values to their string representations.
//  1) All valid LogLevel values (TRACE, DEBUG, INFO, WARN, ERR) produce the expected strings.
//  2) Any out-of-range LogLevel value falls back to "UNKNOWN".
TEST_CASE("toString(LogLevel) mapping", "[LogLevel]")
{
    // SECTION 1: All valid LogLevel values (TRACE, DEBUG, INFO, WARN, ERR) produce the expected strings.
    SECTION("maps each valid enum to its expected string")
    {
        // Valid LogLevel values and their expected string representations
        REQUIRE(std::string(toString(LogLevel::TRACE)) == "TRACE");
        REQUIRE(std::string(toString(LogLevel::DEBUG)) == "DEBUG");
        REQUIRE(std::string(toString(LogLevel::INFO)) == "INFO");
        REQUIRE(std::string(toString(LogLevel::WARN)) == "WARN");
        REQUIRE(std::string(toString(LogLevel::ERR)) == "ERROR");
    }

    // SECTION 2: Any out-of-range LogLevel value falls back to "UNKNOWN".
    SECTION("returns UNKNOWN for out-of-range values")
    {
        auto invalid = static_cast<LogLevel>(0xFF);
        // This value is outside the defined range of LogLevel enum
        REQUIRE(std::string(toString(invalid)) == "UNKNOWN");
    }
}

// Tests Logger::log (and logf) under various conditions:
// 1) early‐exit when no HandlerManager is bound (log)
// 2) early‐exit when no HandlerManager is bound (logf)
// 3) skips disabled handlers
// 4) skips messages below a handler’s severity threshold
// 5) dispatches to the right handler on a subscribed tag
// 6) does NOT invoke a handler on an unsubscribed tag
// 7) invokes a handler on a subscribed tag
// 8) logf() formats and dispatches with a valid manager
TEST_CASE("Logger::log full dispatch & filtering suite", "[Logger]")
{
    // common fixture data for sections 5–7
    Tag CORE("CORE"), BOOT("BOOT"), OTA("OTA");
    std::string lastOutput;
    HandlerManager mgr;

    const Tag *coreTags[] = {&CORE};

    auto TestHandler = [](const LogMessage &msg, void *ctx)
    {
        *static_cast<std::string *>(ctx) = msg.message;
    };

    // 1) log() returns immediately if Logger has no manager
    SECTION("log() does nothing when unbound (no HandlerManager)")
    {
        Logger logger(nullptr);
        Tag T("TEST");
        std::string out;
        logger.log(LogLevel::INFO, &T, "Ignored");
        // HandlerManager is null, so it should not run
        REQUIRE(out.empty());
    }

    // 2) logf() returns immediately if Logger has no manager
    SECTION("logf() does nothing when unbound (no HandlerManager)")
    {
        Logger logger(nullptr);
        Tag T("TEST");
        std::string out;
        logger.logf(LogLevel::ERR, &T, "Err %d", 42);
        // HandlerManager is null, so it should not run
        REQUIRE(out.empty());
    }

    // 3) disabled handlers are skipped
    SECTION("skips disabled handlers")
    {
        HandlerManager mgr;
        Logger logger(&mgr);

        // Subscribe to CORE with a disabled handler
        REQUIRE(mgr.registerHandlerForTags(
            LogLevel::INFO, TestHandler, &lastOutput, coreTags, 1, "DisabledTest"));

        size_t cnt = 0;
        auto entries = mgr.listHandlers(cnt);
        auto *entry = const_cast<HandlerEntry *>(&entries[0]);

        entry->enabled = false;

        logger.log(LogLevel::INFO, &CORE, "Hello");
        // Handler is disabled, so it should not run
        REQUIRE(lastOutput.empty());
    }

    // 4) below‐threshold logs are skipped
    SECTION("skips messages below severity threshold")
    {
        HandlerManager mgr;
        Logger logger(&mgr);
        int count = 0;
        // Subscribe to CORE with a counting handler
        REQUIRE(mgr.registerHandlerForTags(
            LogLevel::WARN, countingHandler, &count, coreTags, 1, "ThreshTest"));

        logger.log(LogLevel::INFO, &CORE, "too low");
        // Handler is below threshold, so it should not run
        REQUIRE(count == 0);
    }

    // 5) dispatches to the right handler for a subscribed tag
    SECTION("dispatches to registered handlers")
    {
        HandlerManager mgr;
        Logger logger(&mgr);
        std::string out;
        auto printHandler = [](auto &msg, void *ctx)
        {
            *static_cast<std::string *>(ctx) = msg.message;
        };
        // Subscribe only to CORE
        REQUIRE(mgr.registerHandlerForTags(
            LogLevel::INFO, printHandler, &out, coreTags, 1, "DispatchTest"));

        logger.log(LogLevel::INFO, &CORE, "System started");
        // Check that the handler was invoked with the expected message
        REQUIRE_FALSE(out.empty());
        // Check that the handler was invoked with the expected message
        REQUIRE(out == "System started");
    }

    // 6) does NOT invoke handler when logging to an unsubscribed tag
    SECTION("does NOT invoke handler on unsubscribed tag")
    {
        HandlerManager mgr;
        Logger logger(&mgr);
        std::string out;

        const Tag *otaTags[] = {&OTA};
        // Register a handler for OTA
        REQUIRE(mgr.registerHandlerForTags(
            LogLevel::INFO, TestHandler, &out, otaTags, 1, "NoDispatchTest"));

        logger.log(LogLevel::INFO, &BOOT, "Boot message");
        // Check that the handler was not invoked
        REQUIRE(out.empty());
    }

    // 7) invokes a handler when logging to a subscribed tag
    SECTION("invokes handler on subscribed tag")
    {
        HandlerManager mgr;
        Logger logger(&mgr);
        std::string out;

        const Tag *otaTags[] = {&OTA};
        // Register a handler for OTA
        REQUIRE(mgr.registerHandlerForTags(
            LogLevel::INFO, TestHandler, &out, otaTags, 1, "PositiveTest"));

        logger.log(LogLevel::INFO, &OTA, "Firmware update started");
        // Check that the handler was invoked
        REQUIRE(out == "Firmware update started");
    }

    // 8) logf() formats and dispatches with a valid manager
    SECTION("logf() formats and dispatches with a valid manager")
    {
        // subscribe to CORE
        std::string out;
        auto logfHandler = [](auto &msg, void *ctx)
        {
            *static_cast<std::string *>(ctx) = msg.message;
        };
        // Register a handler for CORE
        REQUIRE(mgr.registerHandlerForTags(
            LogLevel::INFO,
            logfHandler,
            &out,
            coreTags, 1,
            "LogfTest"));

        // this should invoke your handler with the formatted string
        logger.logf(LogLevel::INFO, &CORE, "Answer is %d", 42);
        // Check that the handler was invoked with the expected message
        REQUIRE(out == "Answer is 42");
    }
}

TEST_CASE("Logger::log timestamp behavior", "[Logger][Timestamp]")
{
    Tag CORE("CORE");
    HandlerManager mgr;
    Logger logger(&mgr);

    // Context to capture the timestamp from the LogMessage delivered to our handler
    struct Ctx
    {
        uint64_t ts;
    } ctx{0};

    // A handler that simply records the msg.timestamp into ctx.ts
    auto captureTs = [](const LogMessage &msg, void *v)
    {
        static_cast<Ctx *>(v)->ts = msg.timestamp;
    };

    const Tag *tsTags[] = {&CORE};
    // Register the timestamp‐capturing handler on CORE
    REQUIRE(mgr.registerHandlerForTags(
        LogLevel::INFO,
        captureTs,
        &ctx,
        tsTags,
        1,
        "TimestampTest"));

    SECTION("uses explicit timestamp when non-zero")
    {
        ctx.ts = 0; // clear before invocation

        // Pass an explicit timestamp
        constexpr uint64_t EXPLICIT_TS = 0x12345678;
        logger.log(LogLevel::INFO, &CORE, "with-explicit", EXPLICIT_TS);

        // Check that the handler was invoked with the expected timestamp
        REQUIRE(ctx.ts == EXPLICIT_TS);
    }

    SECTION("uses timestampProvider when explicitTs == 0")
    {
        ctx.ts = 0; // clear before invocation

        // Install a custom provider returning a known value
        constexpr uint64_t CUSTOM_TS = 0xDEADBEEF;
        logger.setTimestampProvider([]()
                                    { return CUSTOM_TS; });

        // Call with explicitTs == 0 → should pick up CUSTOM_TS
        logger.log(LogLevel::INFO, &CORE, "with-provider", 0);

        // Check that the handler was invoked with the expected timestamp
        REQUIRE(ctx.ts == CUSTOM_TS);
    }
}

// Tests Logger::log when unbound:
// 1) log() does nothing when unbound
// 2) logf() also returns immediately when unbound
TEST_CASE("Logger::log does nothing when unbound (no HandlerManager)",
          "[Logger][nullManager]")
{
    // Create a Logger with no HandlerManager
    Logger logger(nullptr);
    Tag T("TEST");
    std::string out;

    // SECTION 1: log() does nothing when unbound (no HandlerManager)
    SECTION("log() returns immediately when handlerManager == nullptr")
    {
        // This should take the `if (!handlerManager) return;` branch:
        logger.log(LogLevel::INFO, &T, "ignored");
        REQUIRE(out.empty());
    }

    // SECTION 2: logf() also returns immediately when unbound (no HandlerManager)
    SECTION("logf() also returns immediately when unbound")
    {
        logger.logf(LogLevel::ERR, &T, "fmt %d", 42);
        REQUIRE(out.empty());
    }
}

TEST_CASE("Logger::log with valid manager but no handlers",
          "[Logger][nullHandlers]")
{
    // GIVEN a perfectly normal Logger
    HandlerManager mgr;
    Logger logger(&mgr);
    Tag T("ANY");
    std::string out;

    // WHEN I call log() with a valid manager but I haven't registered any handlers
    // THEN we should go past the `if (!handlerManager)` check,
    //     invoke makeMessage + dispatchToHandlers, and simply do nothing.
    // (No crash, no change to 'out', but the branch is taken.)
    logger.log(LogLevel::INFO, &T, "nothing");
    REQUIRE(out.empty());

    // And repeat with logf()
    logger.logf(LogLevel::DEBUG, &T, "fmt %d", 123);
    REQUIRE(out.empty());
}