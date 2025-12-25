#include "catch_amalgamated.hpp"

#include <cstdint>
#include <cstdlib>

#include <utility>
#include <vector>

extern "C" {
#include "util/alphanum.h"
#include "util/math.h"
}

#define SPLAT_STR(str) (str), strlen(str)

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
            REQUIRE(STATUS_OK(strntochr(SPLAT_STR(t.literal), &result)));
            REQUIRE(t.expected == result);
        }

        REQUIRE(strntochr(SPLAT_STR("'ret'"), &result) == Status::MALFORMED_CHARATCER_LITERAL);
    }
}

TEST_CASE("Signed integer from string") {
    int64_t test_value;
    REQUIRE(STATUS_OK(strntoll(SPLAT_STR("1023"), Base::DECIMAL, &test_value)));
    REQUIRE(test_value == 1023);

    REQUIRE(strntoll(SPLAT_STR("0xFFFFFFFFFFFFFFFF"), Base::HEXADECIMAL, &test_value) ==
            Status::SIGNED_INTEGER_OVERFLOW);
    REQUIRE(STATUS_OK(strntoll(SPLAT_STR("0xFF"), Base::HEXADECIMAL, &test_value)));
    REQUIRE(test_value == 0xFF);

    REQUIRE(STATUS_OK(strntoll(SPLAT_STR("0b10011101101"), Base::BINARY, &test_value)));
    REQUIRE(test_value == 0b10011101101);

    REQUIRE(STATUS_OK(strntoll(SPLAT_STR("0o1234567"), Base::OCTAL, &test_value)));
    REQUIRE(test_value == 342391);
}

TEST_CASE("Unsigned integer from string") {
    uint64_t test_value;
    REQUIRE(STATUS_OK(strntoull(SPLAT_STR("1023"), Base::DECIMAL, &test_value)));
    REQUIRE(test_value == 1023);

    REQUIRE(strntoull(SPLAT_STR("0x10000000000000000"), Base::HEXADECIMAL, &test_value) ==
            Status::UNSIGNED_INTEGER_OVERFLOW);
    REQUIRE(STATUS_OK(strntoull(SPLAT_STR("0xFF"), Base::HEXADECIMAL, &test_value)));
    REQUIRE(test_value == 0xFF);

    REQUIRE(STATUS_OK(strntoull(SPLAT_STR("0b10011101101"), Base::BINARY, &test_value)));
    REQUIRE(test_value == 0b10011101101);

    REQUIRE(STATUS_OK(strntoull(SPLAT_STR("0o1234567"), Base::OCTAL, &test_value)));
    REQUIRE(test_value == 342391);
}

TEST_CASE("Size type from string") {
    size_t test_value;
    REQUIRE(STATUS_OK(strntouz(SPLAT_STR("1023"), Base::DECIMAL, &test_value)));
    REQUIRE(test_value == 1023);

    REQUIRE(strntouz(SPLAT_STR("0x10000000000000000"), Base::HEXADECIMAL, &test_value) ==
            Status::SIZE_OVERFLOW);
    REQUIRE(STATUS_OK(strntouz(SPLAT_STR("0xFF"), Base::HEXADECIMAL, &test_value)));
    REQUIRE(test_value == 0xFF);

    REQUIRE(STATUS_OK(strntouz(SPLAT_STR("0b10011101101"), Base::BINARY, &test_value)));
    REQUIRE(test_value == 0b10011101101);

    REQUIRE(STATUS_OK(strntouz(SPLAT_STR("0o1234567"), Base::OCTAL, &test_value)));
    REQUIRE(test_value == 342391);
}

TEST_CASE("Double from string") {
    double test_value;
    REQUIRE(STATUS_OK(strntod(SPLAT_STR("1023"), &test_value)));
    REQUIRE(approx_eq_double(test_value, 1023.0, EPSILON));

    REQUIRE(STATUS_OK(strntod(SPLAT_STR("1023.234612"), &test_value)));
    REQUIRE(approx_eq_double(test_value, 1023.234612, EPSILON));

    REQUIRE(STATUS_OK(strntod(SPLAT_STR("1023.234612e234"), &test_value)));
    REQUIRE(approx_eq_double(test_value, 1023.234612e234, EPSILON));

    // The value doesn't matter here, just checking if it handles too long
    REQUIRE(STATUS_OK(strntod(
        SPLAT_STR("1."
                  "234567891011121314151617181920212223242526272829303132334353637383"
                  "94041424344454647484950515253545556575859606162636465669023487509238749857"),
        &test_value)));
}

TEST_CASE("Malformed numbers") {
    const uint64_t start = 42;
    SECTION("Signing integers") {
        int64_t v = start;
        REQUIRE(strntoll(SPLAT_STR("0b2"), Base::DECIMAL, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(std::cmp_equal(v, start));
        REQUIRE(strntoll(SPLAT_STR("0b2"), Base::BINARY, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(std::cmp_equal(v, start));
        REQUIRE(strntoll(SPLAT_STR("0xG"), Base::HEXADECIMAL, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(std::cmp_equal(v, start));
        REQUIRE(strntoll(SPLAT_STR("0oG"), Base::OCTAL, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(std::cmp_equal(v, start));
    }

    SECTION("Unsigned integers") {
        uint64_t v = start;
        REQUIRE(strntoull(SPLAT_STR("0b2"), Base::DECIMAL, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(v == start);
        REQUIRE(strntoull(SPLAT_STR("0b2"), Base::BINARY, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(v == start);
        REQUIRE(strntoull(SPLAT_STR("0xG"), Base::HEXADECIMAL, &v) ==
                Status::MALFORMED_INTEGER_STR);
        REQUIRE(v == start);
        REQUIRE(strntoull(SPLAT_STR("0oG"), Base::OCTAL, &v) == Status::MALFORMED_INTEGER_STR);
        REQUIRE(v == start);
    }

    SECTION("Floating points") {
        double v = start;
        REQUIRE(strntod(SPLAT_STR("1023.234612e2340"), &v) == Status::FLOAT_OVERFLOW);
        REQUIRE(approx_eq_double(start, v, EPSILON));
        REQUIRE(strntod(SPLAT_STR("1023.23R612e234"), &v) == Status::MALFORMED_FLOAT_STR);
        REQUIRE(approx_eq_double(start, v, EPSILON));
    }
}
