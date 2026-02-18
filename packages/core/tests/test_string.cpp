#include <catch_amalgamated.hpp>

#include "string.hpp"

using namespace conch;

TEST_CASE("Is space") {
    REQUIRE(string::is_space(' '));
    REQUIRE(string::is_space('\t'));
    REQUIRE(string::is_space('\n'));
    REQUIRE(string::is_space('\r'));
    REQUIRE_FALSE(string::is_space('\\'));
}

TEST_CASE("Left trim") {
    REQUIRE(string::trim_left("") == "");
    REQUIRE(string::trim_left("the") == "the");
    REQUIRE(string::trim_left("    the") == "the");
    REQUIRE(string::trim_left("    the    ") == "the    ");
    REQUIRE(string::trim_left("        ") == "");
}

TEST_CASE("Right trim") {
    REQUIRE(string::trim_right("") == "");
    REQUIRE(string::trim_right("the") == "the");
    REQUIRE(string::trim_right("the    ") == "the");
    REQUIRE(string::trim_right("    the    ") == "    the");
    REQUIRE(string::trim_right("        ") == "");
}

TEST_CASE("Trim") {
    REQUIRE(string::trim("") == "");
    REQUIRE(string::trim("the") == "the");
    REQUIRE(string::trim("the    ") == "the");
    REQUIRE(string::trim("    the") == "the");
    REQUIRE(string::trim("    the    ") == "the");
    REQUIRE(string::trim("        ") == "");
}
