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
