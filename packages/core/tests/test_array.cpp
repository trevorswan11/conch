#include <ranges>

#include <catch_amalgamated.hpp>

#include "array.hpp"

namespace conch::tests {

TEST_CASE("View materialization") {
    constexpr auto nums         = std::ranges::views::iota(0, 100);
    constexpr auto materialized = array::materialize_sized_view<100>(nums);
    for (const auto num : nums) { REQUIRE(materialized[num] == num); }
}

TEST_CASE("Array concatenation") {
    constexpr auto               A = std::array{0, 1, 2, 3, 4, 5, 6};
    constexpr auto               B = std::array{7, 8};
    constexpr std::array<int, 0> C = {};

    constexpr auto combined = array::concat(A, B, C);
    for (usize i = 0; i < combined.size(); ++i) { REQUIRE(combined[i] == static_cast<int>(i)); }
}

TEST_CASE("Array combinations") {
    constexpr auto A        = std::array{0, 1, 2};
    constexpr auto expected = std::to_array<std::pair<int, int>>({{0, 1}, {0, 2}, {1, 2}});
    const auto     actual   = array::combinations<int>(A);
    REQUIRE(std::ranges::equal(expected, actual));
}

} // namespace conch::tests
