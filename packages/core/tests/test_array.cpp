#include <catch_amalgamated.hpp>
#include <ranges>

#include "array.hpp"

TEST_CASE("View materialization") {
    constexpr auto nums         = std::ranges::views::iota(0, 100);
    constexpr auto materialized = conch::materialize_sized_view<100>(nums);
    for (const auto num : nums) { REQUIRE(materialized[num] == num); }
}

TEST_CASE("Array concatenation") {
    constexpr auto               A = std::array{0, 1, 2, 3, 4, 5, 6};
    constexpr auto               B = std::array{7, 8};
    constexpr std::array<int, 0> C = {};

    constexpr auto combined = conch::concat_arrays(A, B, C);
    for (auto i = 0uz; i < combined.size(); i++) { REQUIRE(combined[i] == static_cast<int>(i)); }
}
