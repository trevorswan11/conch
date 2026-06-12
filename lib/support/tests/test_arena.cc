#include <array>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "arena.hh"
#include "types.hh"

namespace ghoti::tests {

namespace {

constexpr usize MARKER{42};

struct Large {
    usize                          marker{MARKER};
    std::array<i32, 4UZ * 1'024UZ> _;
};

} // namespace

TEST_CASE("Arena pointer stability") {
    mem::Arena          arena;
    std::vector<Large*> foos;

    // First use
    {
        for (usize i{0}; i < 100; ++i) { foos.emplace_back(arena.make<Large>().get()); }
        for (const auto& foo : foos) { CHECK(foo->marker == MARKER); }
    }

    // Reset and reuse
    {
        arena.reset();
        foos.clear();
        for (usize i{0}; i < 100; ++i) { foos.emplace_back(arena.make<Large>().get()); }
        for (const auto& foo : foos) { CHECK(foo->marker == MARKER); }
    }
}

TEST_CASE("Arena alignment") {
    mem::Arena arena;
    CHECK(*arena.make<bool>(true));
    const auto p{arena.make<void*>(nullptr)};
    CHECK(reinterpret_cast<uptr>(p.get()) % alignof(void*) == 0);
}

TEST_CASE("Arena array construction") {
    mem::Arena arena;
    const auto array{arena.make_span<i32>(10)};
    for (const auto& i : array) { CHECK(i == 0); }
}

} // namespace ghoti::tests
