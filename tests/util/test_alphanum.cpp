#include "catch_amalgamated.hpp"

#include <stdint.h>
#include <stdlib.h>

#include <vector>

extern "C" {
#include "util/alphanum.h"
#include "util/math.h"
}

TEST_CASE("Character utils") {
    SECTION("Checkers") {
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
        REQUIRE_FALSE(is_letter('_'));
        REQUIRE_FALSE(is_letter('1'));
        REQUIRE_FALSE(is_letter(0));

        REQUIRE(is_whitespace(' '));
        REQUIRE(is_whitespace('\t'));
        REQUIRE(is_whitespace('\n'));
        REQUIRE(is_whitespace('\r'));
        REQUIRE_FALSE(is_whitespace(0));
    }

    SECTION("Decode") {
        struct DecodeTestCase {
            const char* literal;
            uint8_t     expected;
        };

        const std::vector<DecodeTestCase> cases = {
            {"'a'", 'a'},
            {"'b'", 'b'},
            {"'\\n'", '\n'},
            {"'\\t'", '\t'},
            {"'\\r'", '\r'},
            {"'\\\\'", '\\'},
            {"'\\''", '\''},
            {"'\\\"'", '"'},
            {"'\\0'", '\0'},
            {"'\\Z'", 'Z'},
        };

        uint8_t result;
        for (const auto& t : cases) {
            REQUIRE(STATUS_OK(strntochr(t.literal, strlen(t.literal), &result)));
            REQUIRE(t.expected == result);
        }

        REQUIRE(strntochr("'ret'", 5, &result) == Status::MALFORMED_CHARATCER_LITERAL);
    }
}

TEST_CASE("Signed integer from string") {
    int64_t test_value;
    REQUIRE(STATUS_OK(strntoll("1023", 4, Base::DECIMAL, &test_value)));
    REQUIRE(test_value == 1023);

    REQUIRE(strntoll("0xFFFFFFFFFFFFFFFF", 18, Base::HEXADECIMAL, &test_value) ==
            Status::SIGNED_INTEGER_OVERFLOW);
    REQUIRE(STATUS_OK(strntoll("0xFF", 4, Base::HEXADECIMAL, &test_value)));
    REQUIRE(test_value == 0xFF);

    REQUIRE(STATUS_OK(strntoll("0b10011101101", 13, Base::BINARY, &test_value)));
    REQUIRE(test_value == 0b10011101101);

    REQUIRE(STATUS_OK(strntoll("0o1234567", 9, Base::OCTAL, &test_value)));
    REQUIRE(test_value == 342391);
}

TEST_CASE("Unsigned integer from string") {
    uint64_t test_value;
    REQUIRE(STATUS_OK(strntoull("1023", 4, Base::DECIMAL, &test_value)));
    REQUIRE(test_value == 1023);

    REQUIRE(strntoull("0x10000000000000000", 19, Base::HEXADECIMAL, &test_value) ==
            Status::UNSIGNED_INTEGER_OVERFLOW);
    REQUIRE(STATUS_OK(strntoull("0xFF", 4, Base::HEXADECIMAL, &test_value)));
    REQUIRE(test_value == 0xFF);

    REQUIRE(STATUS_OK(strntoull("0b10011101101", 13, Base::BINARY, &test_value)));
    REQUIRE(test_value == 0b10011101101);

    REQUIRE(STATUS_OK(strntoull("0o1234567", 9, Base::OCTAL, &test_value)));
    REQUIRE(test_value == 342391);
}

TEST_CASE("Double from string") {
    double test_value;
    REQUIRE(STATUS_OK(strntod("1023", 4, &test_value, malloc, free)));
    REQUIRE(approx_eq_double(test_value, 1023.0, EPSILON));

    REQUIRE(STATUS_OK(strntod("1023.234612", 11, &test_value, malloc, free)));
    REQUIRE(approx_eq_double(test_value, 1023.234612, EPSILON));

    REQUIRE(STATUS_OK(strntod("1023.234612e234", 15, &test_value, malloc, free)));
    REQUIRE(approx_eq_double(test_value, 1023.234612e234, EPSILON));
}

TEST_CASE("Malformed numbers") {
    const uint64_t start = 42;
    SECTION("Signing integers") {
        int64_t v = start;
        REQUIRE(strntoll("0b2", 3, Base::DECIMAL, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(v == start);
        REQUIRE(strntoll("0b2", 3, Base::BINARY, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(v == start);
        REQUIRE(strntoll("0xG", 3, Base::HEXADECIMAL, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(v == start);
        REQUIRE(strntoll("0oG", 3, Base::OCTAL, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(v == start);
    }

    SECTION("Unsigned integers") {
        uint64_t v = start;
        REQUIRE(strntoull("0b2", 3, Base::DECIMAL, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(v == start);
        REQUIRE(strntoull("0b2", 3, Base::BINARY, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(v == start);
        REQUIRE(strntoull("0xG", 3, Base::HEXADECIMAL, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(v == start);
        REQUIRE(strntoull("0oG", 3, Base::OCTAL, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(v == start);
    }

    SECTION("Floating points") {
        double v = start;
        REQUIRE(strntod("1023.234612e2340", 16, &v, malloc, free) == Status::FLOAT_OVERFLOW);
        REQUIRE(approx_eq_double(start, v, EPSILON));
        REQUIRE(strntod("1023.23R612e234", 15, &v, malloc, free) == Status::MALFORMED_FLOAT_STR);
        REQUIRE(approx_eq_double(start, v, EPSILON));
    }
}
