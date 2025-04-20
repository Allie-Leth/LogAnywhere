#define CATCH_CONFIG_MAIN
#include "catch.hpp"


#include "../include/LogMessage.h"
#include "../include/LogLevel.h"

using namespace LogAnywhere;

TEST_CASE("LogMessage stores its values correctly", "[LogMessage]") {
    LogMessage msg;
    msg.level = LogLevel::INFO;
    msg.tag = "TEST";
    msg.message = "This is a test log";
    msg.timestamp = 123456;

    REQUIRE(msg.level == LogLevel::INFO);
    REQUIRE(std::string(msg.tag) == "TEST");
    REQUIRE(std::string(msg.message) == "This is a test log");
    REQUIRE(msg.timestamp == 123456);
}
