#include <algorithm>
#include <iterator>
#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "fixed/hash_map.hh"
#include "helpers/raii_tracker.hh"
#include "types.hh"

namespace ghoti::tests {

TEST_CASE("Metadata helpers") {
    using fixed::detail::Metadata;
    Metadata metadata;

    metadata.open_up();
    CHECK(metadata.is_open());
    metadata.bury();
    CHECK(metadata.is_tombstone());

    metadata.fill(20);
    CHECK(metadata.get_fingerprint() == 20);
    CHECK(metadata.is_used());
    metadata.fill(255);
    REQUIRE(metadata.get_fingerprint() == 127);

    REQUIRE(Metadata::take_fingerprint(300) == 0);
    REQUIRE(Metadata::take_fingerprint(0xFFFFFFFFFFFFFFF) == 7);
    REQUIRE(Metadata::take_fingerprint(0xAFFF5FFFFFFFFFFF) == 87);
    REQUIRE(Metadata::take_fingerprint(0xFFFFFFFFFFFFFFFF) == 127);
}

TEST_CASE("HashMap construction") {
    fixed::HashMap<u16, u32, 4> hm;

    STATIC_CHECK(hm.capacity() == 4);
    CHECK(hm.size() == 0);

    for (const auto& metadata : hm.get_metadata()) {
        CHECK(metadata == fixed::detail::Metadata::make_open_slot());
    }
}

TEST_CASE("HashMap basic usage") {
    fixed::HashMap<u32, u32, 16> hm;

    constexpr u32 insert_count{10};
    for (u32 i = 0; i < insert_count; ++i) { hm.emplace(i, i); }
    CHECK(hm.size() == insert_count);

    for (u32 i = 0; i < insert_count; ++i) {
        CHECK(hm.contains(i));
        const auto val{hm.get_opt(i)};
        REQUIRE(val);
        CHECK(*val == i);
        CHECK(hm.get(i) == i);
    }

    hm.remove(0);
}

TEST_CASE("HashMap helper constructor & clear") {
    auto hm{fixed::make_hash_map(std::pair{0, 0},
                                 std::pair{3, 3},
                                 std::pair{3, 4},
                                 std::pair{4, 4},
                                 std::pair{5, 5},
                                 std::pair{6, 6})};

    STATIC_CHECK(hm.capacity() == 8);
    CHECK(hm.size() == 5);

    CHECK(hm.contains(0));
    CHECK(hm.get(0) == 0);

    CHECK(hm.contains(3));
    CHECK(hm.get(3) == 4);

    hm.clear();
    CHECK(hm.size() == 0);
    for (i32 i = -10; i < 10; ++i) {
        CHECK_FALSE(hm.contains(i));
        CHECK_FALSE(hm.get_opt(i));
    }
}

TEST_CASE("HashMap constexpr operations") {
    constexpr auto hm{fixed::make_hash_map(std::pair{0, 0}, std::pair{3, 3}, std::pair{2, 4})};
    STATIC_CHECK(hm.size() == 3);
    STATIC_CHECK(hm.contains(0));
    STATIC_CHECK(hm.get(0) == 0);
    STATIC_CHECK_FALSE(hm.contains(12));
}

TEST_CASE("HashMap constexpr string view key") {
    using namespace std::string_view_literals;
    constexpr auto hm{fixed::make_hash_map(std::pair{"0"sv, 0},
                                           std::pair{"3"sv, 3},
                                           std::pair{"3"sv, 4},
                                           std::pair{"4"sv, 4},
                                           std::pair{"5"sv, 5},
                                           std::pair{"6"sv, 6})};

    CHECK(hm.contains("0"));
    CHECK(hm.get_opt("3"));
    CHECK(hm.get("3") == 4);
}

using Tracker = helpers::RAIITracker;

TEST_CASE("HashMap destructor correctness") {
    Tracker::reset();
    {
        fixed::HashMap<i32, Tracker, 5> hm;
        hm.emplace(0, 0);
        hm.emplace(1, 0);
    }
    CHECK(Tracker::destruct_count == 2);
}

TEST_CASE("HashMap copy correctness") {
    using HM = fixed::HashMap<i32, Tracker, 3>;
    Tracker::reset();
    {
        HM original;
        original.emplace(0, 0);
        original.emplace(1, 0);

        SECTION("Copy constructor") {
            HM copy{original}; // NOLINT
            CHECK(copy.size() == 2);
            CHECK(Tracker::copy_count == 2);

            // 2 in original, 2 in copy
            CHECK(Tracker::live_count == 4);
        }

        SECTION("Copy assignment") {
            HM assigned;
            assigned = original;
            CHECK(assigned.size() == 2);

            // Copy internally followed by move
            CHECK(Tracker::copy_count == 2);
        }
    }
    CHECK(Tracker::live_count == 0);
}

TEST_CASE("HashMap move correctness") {
    using HM = fixed::HashMap<i32, Tracker, 78>;
    Tracker::reset();
    {
        HM original;
        original.emplace(0, 0);
        original.emplace(1, 0);

        HM destination{std::move(original)};
        CHECK(destination.size() == 2);
        CHECK(original.empty());
        CHECK(Tracker::move_count == 2);
        CHECK(Tracker::live_count == 2);
    }
    CHECK(Tracker::live_count == 0);
}

TEST_CASE("HashMap self assignment") {
    Tracker::reset();
    fixed::HashMap<i32, Tracker, 3> hm;
    hm.emplace(0, 0);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
    hm = hm;
#pragma clang diagnostic pop

    CHECK(hm.size() == 1);
    CHECK(Tracker::live_count == 1);
}

TEST_CASE("HashMap non-const iterator") {
    auto hm{fixed::make_hash_map(std::pair{0, -1},
                                 std::pair{1, -1},
                                 std::pair{2, -1},
                                 std::pair{3, -1},
                                 std::pair{4, -1},
                                 std::pair{5, -1},
                                 std::pair{6, -1},
                                 std::pair{7, -1})};

    SECTION("Full map iteration and update") {
        usize iter_count{0};
        for (auto [key, value] : hm) {
            CHECK(value == -1);
            value = 10;
            iter_count++;
        }
        CHECK(iter_count == hm.size());
        for (auto [key, value] : hm) { CHECK(value == 10); }
    }

    SECTION("Map item removal") {
        usize iter_count{0};
        hm.remove(7);
        for (auto _ : hm) { iter_count++; }
        CHECK(iter_count == hm.size());
    }
}

TEST_CASE("HashMap const iterator") {
    constexpr auto hm{fixed::make_hash_map(
        std::pair{0, -1}, std::pair{1, -1}, std::pair{5, -1}, std::pair{6, -1}, std::pair{7, -1})};

    usize iter_count{0};
    for (auto [key, value] : hm) {
        CHECK(value == -1);
        iter_count++;
    }
    CHECK(iter_count == hm.size());
}

TEST_CASE("HashMap ranges compatibility") {
    STATIC_REQUIRE(std::forward_iterator<fixed::HashMap<u16, u32, 4>::iterator>);
    STATIC_REQUIRE(std::forward_iterator<fixed::HashMap<u16, u32, 4>::const_iterator>);

    constexpr auto hm{fixed::make_hash_map(std::pair{-2, -1}, std::pair{1, -1}, std::pair{5, -1})};
    i32            sum{0};
    usize          iter_count{0};
    std::ranges::for_each(hm, [&sum, &iter_count](auto pair) -> void {
        sum += pair.first;
        iter_count++;
    });

    CHECK(sum == 4);
    CHECK(iter_count == 3);
}

} // namespace ghoti::tests
