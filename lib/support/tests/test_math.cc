#include <type_traits>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "math.hh"
#include "types.hh"

namespace ghoti::tests {

TEMPLATE_TEST_CASE("Ceil power of two sizes", "", u8, u16, u32, u64, usize) { // NOLINT
    CHECK(ceil_power_of_two<TestType>(0) == 1);
    CHECK(ceil_power_of_two<TestType>(2) == 2);
    CHECK(ceil_power_of_two<TestType>(3) == 4);
}

TEST_CASE("Ceil power of two bounds") {
    CHECK(ceil_power_of_two<u16>(4'097) == 8'192);
    CHECK(ceil_power_of_two<u32>(25'702) == 32'768);
    CHECK(ceil_power_of_two<u64>(257) == 512);
    CHECK(ceil_power_of_two<u64>(0xE00000000000000) == 0x1000000000000000);
    CHECK(ceil_power_of_two<u64>(0xF000000000000000) == 0);
}

TEST_CASE("Minimum unsigned int") {
    STATIC_REQUIRE(std::is_same_v<traits::min_uint_for_bits<7>, u8>);
    STATIC_REQUIRE(std::is_same_v<traits::min_uint_for_bits<8>, u8>);
    STATIC_REQUIRE(std::is_same_v<traits::min_uint_for_bits<9>, u16>);
    STATIC_REQUIRE(std::is_same_v<traits::min_uint_for_bits<17>, u32>);
    STATIC_REQUIRE(std::is_same_v<traits::min_uint_for_bits<33>, u64>);
}

} // namespace ghoti::tests
