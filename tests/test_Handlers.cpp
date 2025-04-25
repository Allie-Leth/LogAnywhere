#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../include/LogAnywhere.h"

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <iostream> // for std::cerr

using namespace LogAnywhere;

TEST_CASE("Logger writes to a std::ostringstream (simulated Serial)", "[Handler][Serial]") {
    std::ostringstream serialStream;

    auto serialHandler = [](const LogMessage& msg, void* ctx) {
        auto& out = *static_cast<std::ostringstream*>(ctx);
        out << "[" << toString(msg.level) << "] " << msg.tag << ": " << msg.message << "\n";
    };

    REQUIRE(registerHandler(LogLevel::INFO, serialHandler, &serialStream));

    log(LogLevel::INFO, "SERIAL", "Logged to stream");

    std::string output = serialStream.str();
    REQUIRE(output.find("Logged to stream") != std::string::npos);
    REQUIRE(output.find("[INFO] SERIAL") != std::string::npos);
}

TEST_CASE("Logger writes to a file", "[Handler][File]") {
    const std::string filename = "test_file_output.log";
    std::ofstream fileOut(filename);
    REQUIRE(fileOut.is_open());

    auto fileHandler = [](const LogMessage& msg, void* ctx) {
        auto* file = static_cast<std::ofstream*>(ctx);
        *file << "[" << toString(msg.level) << "] " << msg.tag << ": " << msg.message << "\n";
    };

    REQUIRE(registerHandler(LogLevel::INFO, fileHandler, &fileOut));

    log(LogLevel::INFO, "FILE", "Writing to file");
    fileOut.close();

    std::ifstream fileIn(filename);
    std::string line;
    std::getline(fileIn, line);
    fileIn.close();

    REQUIRE(line.find("Writing to file") != std::string::npos);
    REQUIRE(std::remove(filename.c_str()) == 0); // ✅ Cleanup
}

TEST_CASE("Logger queues logs into a vector (simulated async/memory buffer)", "[Handler][Async]") {
    std::vector<std::string> messageQueue;

    auto queueHandler = [](const LogMessage& msg, void* ctx) {
        auto* queue = static_cast<std::vector<std::string>*>(ctx);
        queue->push_back(std::string(msg.tag) + ": " + msg.message);
    };

    REQUIRE(registerHandler(LogLevel::DEBUG, queueHandler, &messageQueue));

    log(LogLevel::DEBUG, "ASYNC", "Queued 1");
    log(LogLevel::INFO, "ASYNC", "Queued 2");

    REQUIRE(messageQueue.size() == 2);
    REQUIRE(messageQueue[0] == "ASYNC: Queued 1");
    REQUIRE(messageQueue[1] == "ASYNC: Queued 2");
}

TEST_CASE("Logger sends logs to multiple handlers", "[Logger][Handlers][Multiple]") {
    const std::string filename = "test_multi_output.log";
    std::ofstream fileOut(filename);
    std::ostringstream serialOut;

    auto fileHandler = [](const LogMessage& msg, void* ctx) {
        auto* file = static_cast<std::ofstream*>(ctx);
        *file << "[" << toString(msg.level) << "] " << msg.tag << ": " << msg.message << "\n";
    };

    auto serialHandler = [](const LogMessage& msg, void* ctx) {
        auto* stream = static_cast<std::ostringstream*>(ctx);
        *stream << "[" << toString(msg.level) << "] " << msg.tag << ": " << msg.message << "\n";
    };

    REQUIRE(registerHandler(LogLevel::INFO, fileHandler, &fileOut));
    REQUIRE(registerHandler(LogLevel::INFO, serialHandler, &serialOut));

    log(LogLevel::INFO, "TEST", "This should go to both");

    fileOut.close();

    std::ifstream fileIn(filename);
    std::string fileLine;
    std::getline(fileIn, fileLine);
    fileIn.close();

    std::string serialLine = serialOut.str();

    REQUIRE(fileLine.find("This should go to both") != std::string::npos);
    REQUIRE(serialLine.find("This should go to both") != std::string::npos);

    REQUIRE(std::remove(filename.c_str()) == 0); // ✅ Cleanup
}

TEST_CASE("Tag filter prevents handler from receiving unmatched logs", "[Logger][Handlers][Filter]") {
    const std::string filename = "test_tag_filter_output.log";
    std::ofstream fileOut(filename);
    std::ostringstream serialOut;

    auto tagFilter = [](const char* tag, void*) -> bool {
        return std::string(tag) == "MATCH";
    };

    auto fileHandler = [](const LogMessage& msg, void* ctx) {
        auto* file = static_cast<std::ofstream*>(ctx);
        *file << "[" << toString(msg.level) << "] " << msg.tag << ": " << msg.message << "\n";
    };

    auto serialHandler = [](const LogMessage& msg, void* ctx) {
        auto* stream = static_cast<std::ostringstream*>(ctx);
        *stream << "[" << toString(msg.level) << "] " << msg.tag << ": " << msg.message << "\n";
    };

    REQUIRE(registerHandler(LogLevel::INFO, fileHandler, &fileOut, tagFilter));
    REQUIRE(registerHandler(LogLevel::INFO, serialHandler, &serialOut));

    log(LogLevel::INFO, "NO_MATCH", "This should only go to serial");

    fileOut.close();

    std::ifstream fileIn(filename);
    std::string fileLine;
    std::getline(fileIn, fileLine);
    fileIn.close();

    std::string serialLine = serialOut.str();

    REQUIRE(fileLine.empty()); // File handler filtered out
    REQUIRE(serialLine.find("This should only go to serial") != std::string::npos);

    REQUIRE(std::remove(filename.c_str()) == 0); // ✅ Cleanup
}

TEST_CASE("Handler registration and unregistration by ID", "[Handler][Register]") {
    std::ostringstream dummyStream;

    auto dummyHandler = [](const LogMessage& msg, void* ctx) {
        auto* stream = static_cast<std::ostringstream*>(ctx);
        *stream << msg.message;
    };

    // Register a handler
    REQUIRE(registerHandler(LogLevel::INFO, dummyHandler, &dummyStream, nullptr, "DummyHandler"));

    // Manually grab the list and check that the handler was registered
    size_t count = 0;
    const HandlerEntry* handlers = LogAnywhere::handlerManager.listHandlers(count);
    REQUIRE(count >= 1);

    uint16_t id = handlers[count - 1].id; // Get the last registered ID

    // Unregister the handler by ID
    REQUIRE(unregisterHandlerByID(id));

    // Check that handler count decreased
    size_t newCount = 0;
    LogAnywhere::handlerManager.listHandlers(newCount);
    REQUIRE(newCount == count - 1);
}

TEST_CASE("Handler registration and unregistration by Name", "[Handler][Register][Name]") {
    std::ostringstream dummyStream;

    auto dummyHandler = [](const LogMessage& msg, void* ctx) {
        auto* stream = static_cast<std::ostringstream*>(ctx);
        *stream << msg.message;
    };

    const char* handlerName = "NamedHandler";

    // Register a handler with a name
    REQUIRE(registerHandler(LogLevel::DEBUG, dummyHandler, &dummyStream, nullptr, handlerName));

    // Unregister it by name
    REQUIRE(unregisterHandlerByName(handlerName));

    // Confirm no handler with that name remains
    size_t count = 0;
    const HandlerEntry* handlers = LogAnywhere::handlerManager.listHandlers(count);
    for (size_t i = 0; i < count; ++i)
    {
        if (handlers[i].name)
        {
            REQUIRE(std::string(handlers[i].name) != handlerName);
        }
    }
}

TEST_CASE("Handler listing after registration", "[Handler][List]") {
    std::ostringstream dummyStream;

    auto dummyHandler = [](const LogMessage& msg, void* ctx) {
        auto* stream = static_cast<std::ostringstream*>(ctx);
        *stream << msg.message;
    };

    // Count current handlers
    size_t originalCount = 0;
    LogAnywhere::handlerManager.listHandlers(originalCount);

    // Register a new one
    REQUIRE(registerHandler(LogLevel::WARN, dummyHandler, &dummyStream, nullptr, "ListTestHandler"));

    // Check new count
    size_t newCount = 0;
    const HandlerEntry* handlers = LogAnywhere::handlerManager.listHandlers(newCount);

    REQUIRE(newCount == originalCount + 1);

    // Confirm last handler matches what we added
    REQUIRE(handlers[newCount - 1].name != nullptr);
    REQUIRE(std::string(handlers[newCount - 1].name) == "ListTestHandler");
}

TEST_CASE("Handler lookup by ID", "[Handler][Find][ID]") {
    std::ostringstream dummyStream;

    auto dummyHandler = [](const LogMessage& msg, void* ctx) {
        auto* stream = static_cast<std::ostringstream*>(ctx);
        *stream << msg.message;
    };

    // Register a new handler
    REQUIRE(registerHandler(LogLevel::ERR, dummyHandler, &dummyStream, nullptr, "IDLookupHandler"));

    size_t count = 0;
    const HandlerEntry* handlers = LogAnywhere::handlerManager.listHandlers(count);
    REQUIRE(count > 0);

    uint16_t targetID = handlers[count - 1].id;

    // Manually search by ID
    bool found = false;
    for (size_t i = 0; i < count; ++i) {
        if (handlers[i].id == targetID) {
            found = true;
            REQUIRE(std::string(handlers[i].name) == "IDLookupHandler");
            break;
        }
    }

    REQUIRE(found);
}

TEST_CASE("Handler lookup by name", "[Handler][Find][Name]") {
    std::ostringstream dummyStream;

    auto dummyHandler = [](const LogMessage& msg, void* ctx) {
        auto* stream = static_cast<std::ostringstream*>(ctx);
        *stream << msg.message;
    };

    const char* targetName = "NameLookupHandler";

    // Register a new handler
    REQUIRE(registerHandler(LogLevel::DEBUG, dummyHandler, &dummyStream, nullptr, targetName));

    size_t count = 0;
    const HandlerEntry* handlers = LogAnywhere::handlerManager.listHandlers(count);
    REQUIRE(count > 0);

    // Manually search by name
    bool found = false;
    for (size_t i = 0; i < count; ++i) {
        if (handlers[i].name && std::string(handlers[i].name) == targetName) {
            found = true;
            REQUIRE(handlers[i].handler != nullptr);
            break;
        }
    }

    REQUIRE(found);
}
