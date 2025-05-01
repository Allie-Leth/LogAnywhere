#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../include/LogAnywhere.h"

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <iostream> // for std::cerr

using namespace LogAnywhere;

// Tags for all our channels
static Tag TAG_DEFAULT    ("DEFAULT");
static Tag TAG_SERIAL     ("SERIAL");
static Tag TAG_FILE       ("FILE");
static Tag TAG_ASYNC      ("ASYNC");
static Tag TAG_TEST       ("TEST");
static Tag TAG_MATCH      ("MATCH");
static Tag TAG_NO_MATCH   ("NO_MATCH");
static Tag TAG_OTHER("OTHER");

static void countingHandler(const LogMessage& msg, void* ctx) {
    auto* count = static_cast<int*>(ctx);
    ++(*count);
}

// A no-op handler for registration
static void dummyHandler(const LogMessage&, void*) {}

/**
 * @brief Clears both the HandlerManager and resets each Tag's subscriber list.
 */
inline void clearAndVerify()
{
    handlerManager.clearHandlers();

    // reset every Tag's subscriber list
    TAG_DEFAULT   .handlerCount = 0;
    TAG_SERIAL    .handlerCount = 0;
    TAG_FILE      .handlerCount = 0;
    TAG_ASYNC     .handlerCount = 0;
    TAG_TEST      .handlerCount = 0;
    TAG_MATCH     .handlerCount = 0;
    TAG_NO_MATCH  .handlerCount = 0;

    size_t count = 0;
    handlerManager.listHandlers(count);
    REQUIRE(count == 0); // ✅ Confirm no handlers left
}

TEST_CASE("clearHandlers resets registry and ID counter", "[HandlerManager]") {
    // 1) Clear any existing handlers
    handlerManager.clearHandlers();

    // 2) Verify empty
    size_t count = 0;
    auto ptr = handlerManager.listHandlers(count);
    REQUIRE(count == 0);                   // no handlers
    REQUIRE(ptr != nullptr);               // pointer always valid

    // 3) Register two handlers and verify IDs
    const Tag* defaultTags[] = { &TAG_DEFAULT };
    REQUIRE(handlerManager.registerHandlerForTags(
        LogLevel::INFO,  dummyHandler, nullptr, defaultTags, 1));
    REQUIRE(handlerManager.registerHandlerForTags(
        LogLevel::WARN,  dummyHandler, nullptr, defaultTags, 1));

    ptr = handlerManager.listHandlers(count);
    REQUIRE(count == 2);
    REQUIRE(ptr[0].id == 1);
    REQUIRE(ptr[1].id == 2);

    // 4) Clear again
    handlerManager.clearHandlers();
    ptr = handlerManager.listHandlers(count);
    REQUIRE(count == 0);

    // 5) Re-register and verify ID resets to 1
    REQUIRE(handlerManager.registerHandlerForTags(
        LogLevel::ERR, dummyHandler, nullptr, defaultTags, 1));
    ptr = handlerManager.listHandlers(count);
    REQUIRE(count == 1);
    REQUIRE(ptr[0].id == 1);

    clearAndVerify(); // Cleanup
}

TEST_CASE("Logger writes to a std::ostringstream (simulated Serial)", "[Handler][Serial]") {
    std::ostringstream serialStream;

    auto serialHandler = [](const LogMessage& msg, void* ctx) {
        auto& out = *static_cast<std::ostringstream*>(ctx);
        out << "[" << toString(msg.level) << "] "
            << msg.tag << ": " << msg.message << "\n";
    };

    const Tag* serialTags[] = { &TAG_SERIAL };
    REQUIRE(registerHandler(
        LogLevel::INFO, serialHandler, &serialStream, serialTags, 1, "SerialTest"));

    log(LogLevel::INFO, &TAG_SERIAL, "Logged to stream");

    std::string output = serialStream.str();
    REQUIRE(output.find("Logged to stream") != std::string::npos);
    REQUIRE(output.find("[INFO] SERIAL")        != std::string::npos);

    clearAndVerify(); // Cleanup
}

TEST_CASE("Logger writes to a file", "[Handler][File]") {
    const std::string filename = "test_file_output.log";
    std::ofstream     fileOut(filename);
    REQUIRE(fileOut.is_open());

    auto fileHandler = [](const LogMessage& msg, void* ctx) {
        auto* file = static_cast<std::ofstream*>(ctx);
        *file << "[" << toString(msg.level) << "] "
              << msg.tag << ": " << msg.message << "\n";
    };

    const Tag* fileTags[] = { &TAG_FILE };
    REQUIRE(registerHandler(
        LogLevel::INFO, fileHandler, &fileOut, fileTags, 1, "FileTest"));

    log(LogLevel::INFO, &TAG_FILE, "Writing to file");
    fileOut.close();

    std::ifstream fileIn(filename);
    std::string   line;
    std::getline(fileIn, line);
    fileIn.close();

    REQUIRE(line.find("Writing to file") != std::string::npos);
    REQUIRE(std::remove(filename.c_str()) == 0); // ✅ Cleanup

    clearAndVerify(); // Cleanup
}

TEST_CASE("Logger queues logs into a vector (simulated async/memory buffer)", "[Handler][Async]") {
    std::vector<std::string> messageQueue;

    auto queueHandler = [](const LogMessage& msg, void* ctx) {
        auto* queue = static_cast<std::vector<std::string>*>(ctx);
        queue->push_back(std::string(msg.tag) + ": " + msg.message);
    };

    const Tag* asyncTags[] = { &TAG_ASYNC };
    REQUIRE(registerHandler(
        LogLevel::DEBUG, queueHandler, &messageQueue, asyncTags, 1, "AsyncTest"));

    log(LogLevel::DEBUG, &TAG_ASYNC, "Queued 1");
    log(LogLevel::INFO,  &TAG_ASYNC, "Queued 2");

    REQUIRE(messageQueue.size() == 2);
    REQUIRE(messageQueue[0] == "ASYNC: Queued 1");
    REQUIRE(messageQueue[1] == "ASYNC: Queued 2");

    clearAndVerify(); // Cleanup
}

TEST_CASE("Logger sends logs to multiple handlers", "[Logger][Handlers][Multiple]") {
    const std::string filename = "test_multi_output.log";
    std::ofstream     fileOut(filename);
    std::ostringstream serialOut;

    auto fileHandler = [](const LogMessage& msg, void* ctx) {
        auto* file = static_cast<std::ofstream*>(ctx);
        *file << "[" << toString(msg.level) << "] "
              << msg.tag << ": " << msg.message << "\n";
    };
    auto serialHandler = [](const LogMessage& msg, void* ctx) {
        auto* out = static_cast<std::ostringstream*>(ctx);
        *out << "[" << toString(msg.level) << "] "
             << msg.tag << ": " << msg.message << "\n";
    };

    const Tag* bothTags[] = { &TAG_TEST };
    REQUIRE(registerHandler(
        LogLevel::INFO, fileHandler,   &fileOut,   bothTags, 1, "FileMulti"));
    REQUIRE(registerHandler(
        LogLevel::INFO, serialHandler, &serialOut, bothTags, 1, "SerialMulti"));

    log(LogLevel::INFO, &TAG_TEST, "This should go to both");

    fileOut.close();

    std::ifstream fileIn(filename);
    std::string   fileLine;
    std::getline(fileIn, fileLine);
    fileIn.close();
    std::string serialLine = serialOut.str();

    REQUIRE(fileLine   .find("This should go to both") != std::string::npos);
    REQUIRE(serialLine .find("This should go to both") != std::string::npos);
    REQUIRE(std::remove(filename.c_str()) == 0); // ✅ Cleanup

    clearAndVerify(); // Cleanup
}

TEST_CASE("Tag-based filter prevents handler from receiving unmatched logs", "[Logger][Handlers][Filter]") {
    const std::string filename = "test_tag_filter_output.log";
    std::ofstream     fileOut(filename);
    std::ostringstream serialOut;

    // file only subscribes to MATCH
    const Tag* fileTags[]   = { &TAG_MATCH   };
    // serial subscribes to both
    const Tag* serialTags[] = { &TAG_MATCH, &TAG_NO_MATCH };

    auto fileHandler = [](const LogMessage& msg, void* ctx) {
        auto* f = static_cast<std::ofstream*>(ctx);
        *f << "[" << toString(msg.level) << "] "
           << msg.tag << ": " << msg.message << "\n";
    };
    auto serialHandler = [](const LogMessage& msg, void* ctx) {
        auto* s = static_cast<std::ostringstream*>(ctx);
        *s << "[" << toString(msg.level) << "] "
           << msg.tag << ": " << msg.message << "\n";
    };

    REQUIRE(registerHandler(
        LogLevel::INFO, fileHandler,   &fileOut,   fileTags,   1, "FileFilter"));
    REQUIRE(registerHandler(
        LogLevel::INFO, serialHandler, &serialOut, serialTags, 2, "SerialFilter"));

    // emit under NO_MATCH → only serial should fire
    log(LogLevel::INFO, &TAG_NO_MATCH, "This should only go to serial");

    fileOut.close();

    std::ifstream fileIn(filename);
    std::string   fileLine;
    std::getline(fileIn, fileLine);
    fileIn.close();

    std::string serialLine = serialOut.str();
    REQUIRE(fileLine.empty());  
    REQUIRE(serialLine.find("This should only go to serial") != std::string::npos);
    REQUIRE(std::remove(filename.c_str()) == 0);

    clearAndVerify(); // Cleanup
}

TEST_CASE("Handler listing after registration", "[Handler][List]") {
    std::ostringstream dummyStream;

    const Tag* defaultTags[] = { &TAG_DEFAULT };

    // Count current handlers
    size_t originalCount = 0;
    handlerManager.listHandlers(originalCount);

    // Register a new one
    REQUIRE(registerHandler(
        LogLevel::WARN, dummyHandler, &dummyStream, defaultTags, 1, "ListTest"));

    // Check new count
    size_t newCount = 0;
    const HandlerEntry* handlers = handlerManager.listHandlers(newCount);

    REQUIRE(newCount == originalCount + 1);
    REQUIRE(handlers[newCount - 1].name != nullptr);
    REQUIRE(std::string(handlers[newCount - 1].name) == "ListTest");

    clearAndVerify(); // Cleanup
}

TEST_CASE("Handler lookup by ID", "[Handler][Find][ID]") {
    std::ostringstream dummyStream;

    const Tag* defaultTags[] = { &TAG_DEFAULT };

    // Register a new handler
    REQUIRE(registerHandler(
        LogLevel::ERR, dummyHandler, &dummyStream, defaultTags, 1, "IDLookup"));

    size_t count = 0;
    const HandlerEntry* handlers = handlerManager.listHandlers(count);
    REQUIRE(count > 0);

    uint16_t targetID = handlers[count - 1].id;

    bool found = false;
    for (size_t i = 0; i < count; ++i) {
        if (handlers[i].id == targetID) {
            found = true;
            REQUIRE(std::string(handlers[i].name) == "IDLookup");
            break;
        }
    }
    REQUIRE(found);

    clearAndVerify(); // Cleanup
}

TEST_CASE("Handler lookup by name", "[Handler][Find][Name]") {
    std::ostringstream dummyStream;

    const Tag* defaultTags[] = { &TAG_DEFAULT };

    const char* targetName = "NameLookup";

    // Register a new handler
    REQUIRE(registerHandler(
        LogLevel::DEBUG, dummyHandler, &dummyStream, defaultTags, 1, targetName));

    size_t count = 0;
    const HandlerEntry* handlers = handlerManager.listHandlers(count);
    REQUIRE(count > 0);

    bool found = false;
    for (size_t i = 0; i < count; ++i) {
        if (handlers[i].name && std::string(handlers[i].name) == targetName) {
            found = true;
            REQUIRE(handlers[i].handler != nullptr);
            break;
        }
    }
    REQUIRE(found);

    clearAndVerify(); // Cleanup
}

TEST_CASE("HandlerManager initial state and listHandlers", "[HandlerManager]") {
    HandlerManager mgr;
    size_t count = 999;
    const HandlerEntry* entries = mgr.listHandlers(count);
    REQUIRE(count == 0);
    REQUIRE(entries != nullptr);
}

TEST_CASE("registerHandlerForTags increases registry and tag subscriptions", "[HandlerManager]") {
    HandlerManager mgr;
    std::string ctx;
    static Tag T1("T1");
    static Tag T2("T2");
    const Tag* tags[] = { &T1, &T2 };

    bool ok = mgr.registerHandlerForTags(
        LogLevel::DEBUG,
        [](const LogMessage&, void*){},
        &ctx,
        tags, 2,
        "Test"
    );
    REQUIRE(ok);

    size_t count = 0;
    const HandlerEntry* entries = mgr.listHandlers(count);
    REQUIRE(count == 1);
    REQUIRE(std::string(entries[0].name) == "Test");
    
    // Both tags should have one subscriber
    REQUIRE(T1.handlerCount == 1);
    REQUIRE(T2.handlerCount == 1);
}

TEST_CASE("clearHandlers resets registry and does not touch tags", "[HandlerManager]") {
    HandlerManager mgr;
    std::string ctx;
    static Tag T("X");
    const Tag* tags[] = { &T };

    mgr.registerHandlerForTags(LogLevel::INFO,
        [](const LogMessage&, void*){}, &ctx, tags, 1, nullptr);
    REQUIRE(T.handlerCount == 1);

    mgr.clearHandlers();
    size_t count;
    mgr.listHandlers(count);
    REQUIRE(count == 0);
    // Tag still has subscriber list intact (manager does not prune on clear)
    REQUIRE(T.handlerCount == 1);
}

TEST_CASE("deleteHandlerByID prunes subscriptions and registry", "[HandlerManager]") {
    HandlerManager mgr;
    std::string ctx;
    static Tag T("TAG");
    const Tag* tags[] = { &T };

    mgr.registerHandlerForTags(LogLevel::INFO,
        [](const LogMessage&, void*){}, &ctx, tags, 1, "ToRemove");
    size_t count;
    mgr.listHandlers(count);
    uint16_t id = mgr.listHandlers(count)[0].id;
    REQUIRE(T.handlerCount == 1);

    bool removed = mgr.deleteHandlerByID(id);
    REQUIRE(removed);
    mgr.listHandlers(count);
    REQUIRE(count == 0);
    REQUIRE(T.handlerCount == 0);
}

TEST_CASE("deleteHandlerByName prunes subscriptions and registry", "[HandlerManager]") {
    HandlerManager mgr;
    std::string ctx;
    static Tag T("TAG");
    const Tag* tags[] = { &T };

    mgr.registerHandlerForTags(LogLevel::WARN,
        [](const LogMessage&, void*){}, &ctx, tags, 1, "MyName");
    size_t count;
    mgr.listHandlers(count);
    REQUIRE(count == 1);
    REQUIRE(T.handlerCount == 1);

    bool removed = mgr.deleteHandlerByName("MyName");
    REQUIRE(removed);
    mgr.listHandlers(count);
    REQUIRE(count == 0);
    REQUIRE(T.handlerCount == 0);
}

TEST_CASE("deleteHandler* returns false if not found", "[HandlerManager]") {
    HandlerManager mgr;
    REQUIRE_FALSE(mgr.deleteHandlerByID(999));
    REQUIRE_FALSE(mgr.deleteHandlerByName("NoSuch"));
}

TEST_CASE("deleteHandlerByID removes handler and unsubscribes from Tag", "[HandlerManager][DeleteByID]") {
    clearHandlers();
    TAG_DEFAULT.handlerCount = 0;
    TAG_OTHER.handlerCount   = 0;

    int defaultCount = 0;
    int otherCount   = 0;
    const Tag* defaultTags[] = { &TAG_DEFAULT };
    const Tag* otherTags[]   = { &TAG_OTHER   };

    // Register two handlers
    REQUIRE(registerHandler(LogLevel::INFO, countingHandler, &defaultCount, defaultTags, 1, "default"));
    REQUIRE(registerHandler(LogLevel::INFO, countingHandler, &otherCount,   otherTags,   1, "other"));

    // Delete the first handler by its ID
    size_t count = 0;
    auto list = handlerManager.listHandlers(count);
    uint16_t idToDelete = list[0].id;
    REQUIRE(deleteHandlerByID(idToDelete));

    // Logging to both tags should only invoke second handler
    log(LogLevel::INFO, &TAG_DEFAULT, "test");
    log(LogLevel::INFO, &TAG_OTHER,   "test");
    REQUIRE(defaultCount == 0);
    REQUIRE(otherCount   == 1);

    // Deleting non-existent ID returns false
    REQUIRE(!deleteHandlerByID(0xFFFF));
}


TEST_CASE("deleteHandlerByName removes handler by name", "[HandlerManager][DeleteByName]") {
    clearHandlers();
    TAG_DEFAULT.handlerCount = 0;

    int count = 0;
    const Tag* tags[] = { &TAG_DEFAULT };

    REQUIRE(registerHandler(LogLevel::INFO, countingHandler, &count, tags, 1, "to_remove"));
    REQUIRE(registerHandler(LogLevel::INFO, countingHandler, &count, tags, 1, "keep"));

    REQUIRE(deleteHandlerByName("to_remove"));
    log(LogLevel::INFO, &TAG_DEFAULT, "hello");
    REQUIRE(count == 1);

    // Removing again returns false
    REQUIRE(!deleteHandlerByName("to_remove"));
}

TEST_CASE("registerHandlerForTags fails when capacity exceeded", "[HandlerManager][Capacity]") {
    clearHandlers();
    TAG_DEFAULT.handlerCount = 0;

    const Tag* tags[] = { &TAG_DEFAULT };
    int dummy = 0;

    // Fill to max
    for (size_t i = 0; i < LOGANYWHERE_MAX_HANDLERS; ++i) {
        REQUIRE(registerHandler(LogLevel::INFO, countingHandler, &dummy, tags, 1));
    }
    // Next should fail
    REQUIRE(!registerHandler(LogLevel::INFO, countingHandler, &dummy, tags, 1));
}

TEST_CASE("handler level threshold filters messages", "[Logger][Level]") {
    clearHandlers();
    TAG_DEFAULT.handlerCount = 0;

    int count = 0;
    const Tag* tags[] = { &TAG_DEFAULT };

    REQUIRE(registerHandler(LogLevel::WARN, countingHandler, &count, tags, 1));

    log(LogLevel::DEBUG, &TAG_DEFAULT, "low");
    REQUIRE(count == 0);

    log(LogLevel::WARN, &TAG_DEFAULT, "equal");
    log(LogLevel::ERR,  &TAG_DEFAULT, "high");
    REQUIRE(count == 2);
}

TEST_CASE("formatted logf works correctly", "[Logger][Format]") {
    clearHandlers();
    TAG_DEFAULT.handlerCount = 0;

    std::ostringstream out;
    auto fmtHandler = [](const LogMessage& msg, void* ctx) {
        auto& os = *static_cast<std::ostringstream*>(ctx);
        os << msg.message;
    };
    const Tag* tags[] = { &TAG_DEFAULT };
    REQUIRE(registerHandler(LogLevel::INFO, fmtHandler, &out, tags, 1));

    logf(LogLevel::INFO, &TAG_DEFAULT, "%d + %d = %d", 2, 3, 5);
    REQUIRE(out.str() == "2 + 3 = 5");
}