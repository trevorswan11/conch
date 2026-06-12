#include <algorithm>
#include <iterator>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <gsl/pointers>
#include <gsl/span>

#include "fixed/vector.hh"
#include "helpers/raii_tracker.hh"
#include "memory.hh"
#include "type_traits.hh"
#include "types.hh"

namespace ghoti::tests {

TEST_CASE("StaticVector type checks") {
    STATIC_REQUIRE(traits::TriviallyDestructible<fixed::Vector<gsl::not_null<i32*>, 4>>);
    STATIC_REQUIRE_FALSE(traits::TriviallyDestructible<fixed::Vector<mem::Box<i32>, 4>>);
}

TEST_CASE("StaticVector basic usage") {
    fixed::Vector<i32, 5> vec;
    CHECK(vec.empty());
    CHECK(vec.size() == 0);

    for (i32 i = 0; i < 5; ++i) { vec.push_back(i * 10); }
    CHECK(vec.size() == 5);
    CHECK_FALSE(vec.empty());
}

TEST_CASE("StaticVector iteration") {
    fixed::Vector<i32, 5> vec{1, 2, 3};
    i32                   count{0};
    i32                   sum{0};
    for (i32 val : vec) {
        sum += val;
        count++;
    }

    CHECK(sum == 6);
    CHECK(count == 3);
    CHECK(vec.end() == vec.begin() + count);
}

TEST_CASE("StaticVector indexing") {
    fixed::Vector<i32, 3> vec{10, 20, 30};
    CHECK(vec[0] == 10);
    CHECK(vec[1] == 20);
    CHECK(vec[2] == 30);
}

TEST_CASE("StaticVector with non-trivial type") {
    SECTION("Contrived example") {
        struct Point {
            i32 x, y;
            Point(i32 x, i32 y) : x{x}, y{y} {}
        };

        fixed::Vector<Point, 2> points;
        points.emplace_back(5, 10);
        CHECK(points[0].x == 5);
        CHECK(points[0].y == 10);
    }

    SECTION("NonNull usage") {
        fixed::Vector<gsl::not_null<i32*>, 2> ptrs;
        i32                                   v{22};
        ptrs.emplace_back(&v);
        CHECK(ptrs[0] == &v);
        CHECK(*ptrs[0] == v);
    }
}

TEST_CASE("StaticVector span conversion") {
    fixed::Vector<i32, 4> vec{1, 2};
    gsl::span<i32>        s = vec;
    CHECK(s.size() == 2);
    CHECK(std::ranges::equal(s, vec));
}

TEST_CASE("Vector constexpr operations") {
    constexpr auto vec{fixed::Vector<i32, 4>{1, 2, 3}};
    STATIC_CHECK(vec.size() == 3);
    STATIC_CHECK(vec[0] == 1);
    STATIC_CHECK(vec[1] == 2);
    STATIC_CHECK(vec[2] == 3);
}

using Tracker = helpers::RAIITracker;

TEST_CASE("StaticVector destructor correctness") {
    Tracker::reset();
    {
        fixed::Vector<Tracker, 5> vec;
        vec.emplace_back(0);
        vec.emplace_back(0);
    }
    CHECK(Tracker::destruct_count == 2);
}

TEST_CASE("StaticVector copy correctness") {
    Tracker::reset();
    {
        fixed::Vector<Tracker, 3> original;
        original.emplace_back(0);
        original.emplace_back(0);

        SECTION("Copy constructor") {
            fixed::Vector<Tracker, 3> copy = original; // NOLINT
            CHECK(copy.size() == 2);
            CHECK(Tracker::copy_count == 2);

            // 2 in original, 2 in copy
            CHECK(Tracker::live_count == 4);
        }

        SECTION("Copy assignment") {
            fixed::Vector<Tracker, 3> assigned;
            assigned = original;
            CHECK(assigned.size() == 2);

            // Copy internally followed by move
            CHECK(Tracker::copy_count == 2);
        }
    }
    CHECK(Tracker::live_count == 0);
}

TEST_CASE("StaticVector move correctness") {
    Tracker::reset();
    {
        fixed::Vector<Tracker, 3> original;
        original.emplace_back(0);
        original.emplace_back(0);

        fixed::Vector<Tracker, 3> destination = std::move(original);
        CHECK(destination.size() == 2);
        CHECK(original.empty());
        CHECK(Tracker::move_count == 2);
        CHECK(Tracker::live_count == 2);
    }
    CHECK(Tracker::live_count == 0);
}

TEST_CASE("Vector self assignment") {
    Tracker::reset();
    fixed::Vector<Tracker, 3> vec;
    vec.emplace_back(0);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
    vec = vec;
#pragma clang diagnostic pop

    CHECK(vec.size() == 1);
    CHECK(Tracker::live_count == 1);
}

TEST_CASE("StaticVector ranges compatibility") {
    STATIC_REQUIRE(std::forward_iterator<fixed::Vector<gsl::not_null<i32*>, 4>::iterator>);
    STATIC_REQUIRE(std::forward_iterator<fixed::Vector<gsl::not_null<i32*>, 4>::const_iterator>);

    constexpr auto vec{fixed::Vector<i32, 4>{1, 2, 3}};
    i32            sum{0};
    usize          iter_count{0};
    std::ranges::for_each(vec, [&sum, &iter_count](i32 val) -> void {
        sum += val;
        iter_count++;
    });

    CHECK(iter_count == 3);
    CHECK(sum == 6);
}

} // namespace ghoti::tests
