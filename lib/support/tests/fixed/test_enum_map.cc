#include <algorithm>
#include <iterator>

#include <catch2/catch_test_macros.hpp>

#include "fixed/enum_map.hh"
#include "helpers/enum.hh"
#include "option.hh"
#include "types.hh"

namespace ghoti::tests {

using helpers::MockEnum;
using helpers::MockNegativeEnum;
using helpers::MockPositiveEnum;
using helpers::NonMonotonicEnum;

TEST_CASE("Standard enum map") {
    fixed::EnumMap<MockEnum, opt::Option<int>> map;
    CHECK(map.size() == 4);
    for (const auto& item : map) { CHECK_FALSE(item); }

    map[MockEnum::A] = 4;
    for (const auto& item : map) {
        if (item) { CHECK(item == 4); }
    }

    SECTION("Automatic optional getting") {
        const auto present_opt{map.get_opt(MockEnum::A)};
        CHECK(present_opt);
        CHECK(present_opt == 4);

        const auto missing_opt{map.get_opt(MockEnum::B)};
        CHECK_FALSE(missing_opt);
    }
}

TEST_CASE("Positive enum map") {
    fixed::EnumMap<MockPositiveEnum, usize*> map;
    CHECK(map.size() == 4);
    for (const auto& item : map) { CHECK(item == nullptr); }

    usize v{1};
    map[MockPositiveEnum::A] = &v;
    for (const auto& item : map) {
        if (item != nullptr) { CHECK(*item == 1); }
    }

    SECTION("Automatic optional getting") {
        const auto present_opt{map.get_opt(MockPositiveEnum::A)};
        CHECK(**present_opt == 1);

        const auto missing_opt{map.get_opt(MockPositiveEnum::B)};
        CHECK_FALSE(missing_opt);
    }
}

TEST_CASE("Negative enum map") {
    fixed::EnumMap<MockNegativeEnum, bool> map{true};
    CHECK(map.size() == 4);
    for (const auto& item : map) { CHECK(item); }

    map[MockNegativeEnum::A] = false;
    CHECK_FALSE(map[MockNegativeEnum::A]);
    CHECK(map[MockNegativeEnum::B]);
    CHECK(map[MockNegativeEnum::C]);
    CHECK(map[MockNegativeEnum::D]);
}

TEST_CASE("Non-monotonic enum map") {
    fixed::EnumMap<NonMonotonicEnum, usize> map{0xDEADBEEF};
    CHECK(map.size() == 4);

    map[NonMonotonicEnum::D] = 0xC0FFEE;
    CHECK(map[NonMonotonicEnum::A] == 0xDEADBEEF);
    CHECK(map[NonMonotonicEnum::B] == 0xDEADBEEF);
    CHECK(map[NonMonotonicEnum::C] == 0xDEADBEEF);
    CHECK(map[NonMonotonicEnum::D] == 0xC0FFEE);
}

TEST_CASE("EnumMap ranges compatibility") {
    using EnumMap = fixed::EnumMap<NonMonotonicEnum, usize>;
    STATIC_REQUIRE(std::forward_iterator<EnumMap::iterator>);
    STATIC_REQUIRE(std::forward_iterator<EnumMap::const_iterator>);

    constexpr EnumMap map{0xDEADBEEF};
    std::ranges::for_each(map, [](usize value) -> void { CHECK(value == 0xDEADBEEF); });
}

} // namespace ghoti::tests
