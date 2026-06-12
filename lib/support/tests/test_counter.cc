#include <catch2/catch_test_macros.hpp>

#include "counter.hh"
#include "types.hh"

namespace ghoti::tests {

TEST_CASE("Default counter") {
    DefaultCounter c;

    CHECK(c == 0);
    SECTION("Single guard") {
        const auto g{c.guard()};
        CHECK(c == 1);
    }
    CHECK(c == 0);

    SECTION("Nested guards") {
        const auto g1{c.guard()};
        {
            const auto g2{c.guard()};
            CHECK(c == 2);
        }
        CHECK(c == 1);
    }
    CHECK(c == 0);
}

TEST_CASE("Counter operators") {
    Counter<i32> c;
    CHECK(c == 0);
    CHECK(c <= 0);
    CHECK(c <= 10);
    CHECK(c >= 0);
    CHECK(c >= -10);

    CHECK_FALSE(c);
    const auto g{c.guard()};
    CHECK(c);
}

} // namespace ghoti::tests
