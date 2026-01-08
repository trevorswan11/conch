#include "catch_amalgamated.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "util/math.h"
}

// NOLINTBEGIN
MAX_FN(int, int)
MAX_FN(double, double)
MAX_FN(size_t, size_t)
// NOLINTEND

TEST_CASE("max_int") {
    REQUIRE(max_int(1, 5) == 5);
    REQUIRE(max_int(3, 1, 2, 3) == 3);
    REQUIRE(max_int(5, -10, -5, 0, 5, 10) == 10);
}

TEST_CASE("max_double and approx_eq") {
    REQUIRE(approx_eq_double(max_double(2, 1.5, 2.5), 2.5, EPSILON));
    REQUIRE(approx_eq_double(max_double(3, -1.0, -2.0, -0.5), -0.5, EPSILON));

    REQUIRE(approx_eq_double(-0.49, -0.5, 0.1));
    REQUIRE_FALSE(approx_eq_double(-0.49, -0.5, 0.001));
    REQUIRE(approx_eq_float(-0.49F, -0.5F, 0.1F));
    REQUIRE_FALSE(approx_eq_float(-0.49F, -0.5F, 0.001F));

    REQUIRE_FALSE(approx_eq_float(nanf(""), -0.5F, 0.001F));
    REQUIRE_FALSE(approx_eq_float(-0.49F, nanf(""), 0.001F));
    REQUIRE_FALSE(approx_eq_double(nan(""), -0.5, 0.1));
    REQUIRE_FALSE(approx_eq_double(-0.49, nan(""), 0.1));
}

TEST_CASE("max_size_t") {
    REQUIRE(max_size_t(3, 1, 5, 3) == 5);
    REQUIRE(max_size_t(1, SIZE_MAX) == SIZE_MAX);
}

TEST_CASE("ceil_power_of_two_32") {
    REQUIRE(ceil_power_of_two_32(0) == 1);
    REQUIRE(ceil_power_of_two_32(1) == 1);
    REQUIRE(ceil_power_of_two_32(2) == 2);
    REQUIRE(ceil_power_of_two_32(3) == 4);
    REQUIRE(ceil_power_of_two_32(5) == 8);
    REQUIRE(ceil_power_of_two_32(0x7FFFFFFF) == 0x80000000);
}

TEST_CASE("ceil_power_of_two_64") {
    REQUIRE(ceil_power_of_two_64(0) == 1);
    REQUIRE(ceil_power_of_two_64(1) == 1);
    REQUIRE(ceil_power_of_two_64(2) == 2);
    REQUIRE(ceil_power_of_two_64(3) == 4);
    REQUIRE(ceil_power_of_two_64(5) == 8);
    REQUIRE(ceil_power_of_two_64(0x7FFFFFFFFFFFFFFF) == 0x8000000000000000);
}

TEST_CASE("is_power_of_two") {
    REQUIRE(is_power_of_two(0) == false);
    REQUIRE(is_power_of_two(1) == true);
    REQUIRE(is_power_of_two(2) == true);
    REQUIRE(is_power_of_two(3) == false);
    REQUIRE(is_power_of_two(4) == true);
    REQUIRE(is_power_of_two(5) == false);
    REQUIRE(is_power_of_two(16) == true);
    REQUIRE(is_power_of_two(31) == false);
    REQUIRE(is_power_of_two(64) == true);
}
