#include "catch_amalgamated.hpp"

extern "C" {
#include "util/alphanum.h"
}

TEST_CASE("Character utils") {
    REQUIRE(is_digit('0'));
    REQUIRE(is_digit('5'));
    REQUIRE(is_digit('9'));
    REQUIRE_FALSE(is_digit('a'));
    REQUIRE_FALSE(is_digit(0));

    REQUIRE(is_letter('a'));
    REQUIRE(is_letter('j'));
    REQUIRE(is_letter('z'));
    REQUIRE(is_letter('A'));
    REQUIRE(is_letter('J'));
    REQUIRE(is_letter('Z'));
    REQUIRE(is_letter('_'));
    REQUIRE_FALSE(is_letter('1'));
    REQUIRE_FALSE(is_letter(0));

    REQUIRE(is_whitespace(' '));
    REQUIRE(is_whitespace('\t'));
    REQUIRE(is_whitespace('\n'));
    REQUIRE(is_whitespace('\r'));
    REQUIRE_FALSE(is_whitespace(0));
}
