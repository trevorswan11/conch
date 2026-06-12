#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "helpers/inheritance.hh"
#include "memory.hh"

namespace ghoti::tests {

using OkBox     = mem::Box<bool>;
using NotBox    = bool;
using CustomBox = mem::Box<bool, void (*)(bool*)>;

TEST_CASE("Basic box construction") {
    auto b{mem::make_box<int>(42)};
    CHECK(*b == 42);
}

TEST_CASE("Box upcasting") {
    auto                    d{mem::make_box<helpers::Derived>()};
    mem::Box<helpers::Base> b{std::move(d)};
    CHECK(b->x == 10);
}

TEST_CASE("Box release") {
    auto b{mem::make_box<int>(10)};
    int* raw = b.release();
    CHECK(raw != nullptr);
    delete raw;
}

} // namespace ghoti::tests
