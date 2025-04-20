#define CATCH_CONFIG_MAIN
#include "catch.hpp"


#include "../include/Logger.h"
#include "../include/LogMessage.h"
#include "../include/LogLevel.h"

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <iostream> // Include for std::cerr

using namespace LogAnywhere;

TEST_CASE("Logger writes to a std::string stream (simulated Serial)", "[Handler][Serial]") {
    Logger logger;
    std::ostringstream serialStream;

    auto SerialHandler = [](const LogMessage& msg, void* ctx) {
        auto& out = *static_cast<std::ostringstream*>(ctx);
        out << "[" << toString(msg.level) << "] " << msg.tag << ": " << msg.message << "\n";
    };

    logger.registerHandler(LogLevel::INFO, SerialHandler, &serialStream);
    logger.log(LogLevel::INFO, "SERIAL", "Logged to stream");

    std::string output = serialStream.str();
    REQUIRE(output.find("Logged to stream") != std::string::npos);
    REQUIRE(output.find("[INFO] SERIAL") != std::string::npos);
}

TEST_CASE("Logger writes to a file", "[Handler][File]") {
    Logger logger;
    const std::string filename = "test_file_output.log";

    std::ofstream fileOut(filename);
    std::cerr << "Opening file: " << filename << std::endl;
    std::cerr << "File is open: " << fileOut.is_open() << std::endl;

    REQUIRE(fileOut.is_open());

auto FileHandler = [](const LogMessage& msg, void* ctx) {
    auto* file = static_cast<std::ofstream*>(ctx);
    std::cerr << "Writing to file from handler: " << msg.message << std::endl;
    *file << "[" << toString(msg.level) << "] " << msg.tag << ": " << msg.message << "\n";
};

    logger.registerHandler(LogLevel::INFO, FileHandler, &fileOut);
    logger.log(LogLevel::INFO, "FILE", "Writing to file");
    fileOut.close();

    std::ifstream fileIn(filename);
    std::string line;
    std::getline(fileIn, line);
    fileIn.close();

    REQUIRE(line.find("Writing to file") != std::string::npos);

    // ✅ Cleanup: Delete the test file
    REQUIRE(std::remove(filename.c_str()) == 0);  // 0 = success
}


TEST_CASE("Logger queues logs into a vector (simulated async/memory buffer)", "[Handler][Async]") {
    Logger logger;
    std::vector<std::string> messageQueue;

    auto QueueHandler = [](const LogMessage& msg, void* ctx) {
        auto* queue = static_cast<std::vector<std::string>*>(ctx);
        queue->push_back(std::string(msg.tag) + ": " + msg.message);
    };

    logger.registerHandler(LogLevel::DEBUG, QueueHandler, &messageQueue);

    logger.log(LogLevel::DEBUG, "ASYNC", "Queued 1");
    logger.log(LogLevel::INFO, "ASYNC", "Queued 2");

    REQUIRE(messageQueue.size() == 2);
    REQUIRE(messageQueue[0] == "ASYNC: Queued 1");
    REQUIRE(messageQueue[1] == "ASYNC: Queued 2");
}

TEST_CASE("Logger sends to multiple handlers", "[Logger][Handlers][Multiple]") {
    Logger logger;

    const std::string filename = "test_multi_output.log";
    std::ofstream fileOut(filename);
    std::ostringstream serialOut;

    // File handler
    auto FileHandler = [](const LogMessage& msg, void* ctx) {
        auto* file = static_cast<std::ofstream*>(ctx);
        *file << "[" << toString(msg.level) << "] " << msg.tag << ": " << msg.message << "\n";
        file->flush();
    };

    // Serial handler
    auto SerialHandler = [](const LogMessage& msg, void* ctx) {
        auto* stream = static_cast<std::ostringstream*>(ctx);
        *stream << "[" << toString(msg.level) << "] " << msg.tag << ": " << msg.message << "\n";
    };

    REQUIRE(logger.registerHandler(LogLevel::INFO, FileHandler, &fileOut));
    REQUIRE(logger.registerHandler(LogLevel::INFO, SerialHandler, &serialOut));

    logger.log(LogLevel::INFO, "TEST", "This should go to both");

    fileOut.close();

    // Check file
    std::ifstream fileIn(filename);
    std::string fileLine;
    std::getline(fileIn, fileLine);
    fileIn.close();

    // Check serial
    std::string serialLine = serialOut.str();

    REQUIRE(fileLine.find("This should go to both") != std::string::npos);
    REQUIRE(serialLine.find("This should go to both") != std::string::npos);

    std::remove(filename.c_str()); // Clean up
}

TEST_CASE("Logger tag filter prevents one handler", "[Logger][Handlers][Filter]") {
    Logger logger;

    const std::string filename = "test_tag_filter_output.log";
    std::ofstream fileOut(filename);
    std::ostringstream serialOut;

    // Only allow logs with tag "MATCH"
    auto TagFilter = [](const char* tag, void*) -> bool {
        return std::string(tag) == "MATCH";
    };

    auto FileHandler = [](const LogMessage& msg, void* ctx) {
        auto* file = static_cast<std::ofstream*>(ctx);
        *file << "[" << toString(msg.level) << "] " << msg.tag << ": " << msg.message << "\n";
        file->flush();
    };

    auto SerialHandler = [](const LogMessage& msg, void* ctx) {
        auto* stream = static_cast<std::ostringstream*>(ctx);
        *stream << "[" << toString(msg.level) << "] " << msg.tag << ": " << msg.message << "\n";
    };

    REQUIRE(logger.registerHandlerFiltered(LogLevel::INFO, FileHandler, &fileOut, TagFilter));
    REQUIRE(logger.registerHandler(LogLevel::INFO, SerialHandler, &serialOut));

    logger.log(LogLevel::INFO, "NO_MATCH", "Should only go to serial");

    fileOut.close();

    std::ifstream fileIn(filename);
    std::string fileLine;
    std::getline(fileIn, fileLine);
    fileIn.close();

    std::string serialLine = serialOut.str();

    REQUIRE(fileLine.empty()); // filtered out
    REQUIRE(serialLine.find("Should only go to serial") != std::string::npos);

    std::remove(filename.c_str()); // Clean up
}
