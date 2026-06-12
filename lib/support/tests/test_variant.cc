#include <string>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "helpers/dummy.hh"
#include "helpers/raii_tracker.hh"
#include "option.hh"
#include "types.hh"
#include "variant.hh"

namespace ghoti::tests {

using Foo     = helpers::Foo;
using Bar     = helpers::Bar;
using Baz     = helpers::Baz;
using FBB     = Variant<Foo, Bar, Baz>;
using Tracker = helpers::RAIITracker;

TEST_CASE("Variant default construction activates first alternative") {
    Variant<Foo, Bar> v;
    CHECK(v.is<Foo>());
    CHECK(v.index() == 0UZ);
}

TEST_CASE("Variant implicit construction from alternative type") {
    FBB v{Foo{42}};
    CHECK(v.is<Foo>());
    CHECK(v.as<Foo>().value == 42);
}

TEST_CASE("Variant in-place construction") {
    FBB v{std::in_place_type<Bar>, "hello"};
    CHECK(v.is<Bar>());
    CHECK(v.as<Bar>().value == "hello");
}

TEST_CASE("Variant::emplace<T> changes active alternative") {
    FBB v{Foo{1}};
    v.emplace<Bar>("emplaced");
    CHECK(v.is<Bar>());
    CHECK(v.as<Bar>().value == "emplaced");
}

TEST_CASE("Variant::emplace<T> calls destructor on old value") {
    Tracker::reset();
    {
        Variant<Tracker, Foo> v = Tracker{0};
        CHECK(Tracker::live_count == 1);
        v.emplace<Foo>(99);
        CHECK(Tracker::live_count == 0);
    }
}

TEST_CASE("Variant::is<T>") {
    FBB v{Bar{"x"}};
    CHECK(v.is<Bar>());
    CHECK_FALSE(v.is<Foo>());
    CHECK_FALSE(v.is<Baz>());
}

TEST_CASE("Variant::index") {
    CHECK(FBB{Foo{}}.index() == 0UZ);
    CHECK(FBB{Bar{}}.index() == 1UZ);
    CHECK(FBB{Baz{}}.index() == 2UZ);
}

TEST_CASE("Variant::as<T> returns mutable reference") {
    FBB v{Foo{1}};
    v.as<Foo>().value = 99;
    CHECK(v.as<Foo>().value == 99);
}

TEST_CASE("Variant::as<T> returns const reference on const Variant") {
    const FBB  v{Bar{"const"}};
    const Bar& b{v.as<Bar>()};
    CHECK(b.value == "const");
}

TEST_CASE("Variant::as<T> moves out of rvalue Variant") {
    FBB v{Bar{"moved"}};
    Bar b{std::move(v).as<Bar>()};
    CHECK(b.value == "moved");
}

TEST_CASE("Variant::as_opt<T> returns engaged Option for active type") {
    FBB  v{Baz{1.5}};
    auto opt{v.as_opt<Baz>()};
    REQUIRE(opt.has_value());
    CHECK(opt->value == 1.5);
}

TEST_CASE("Variant::as_opt<T> returns empty Option for inactive type") {
    FBB v{Foo{1}};
    CHECK_FALSE(v.as_opt<Bar>().has_value());
}

TEST_CASE("Variant::as_opt<T> const yields const ref") {
    const FBB               v{Bar{"ro"}};
    opt::Option<const Bar&> opt = v.as_opt<Bar>();
    REQUIRE(opt.has_value());
    CHECK(opt->value == "ro");
}

TEST_CASE("Variant::as_opt<T> supports transform()") {
    FBB  v{Bar{"transform"}};
    auto len{v.as_opt<Bar>().transform([](const Bar& b) -> usize { return b.value.size(); })};
    REQUIRE(len.has_value());
    CHECK(*len == 9UZ);
}

TEST_CASE("Variant::visit variadic lambda form with non-void return") {
    FBB v{Foo{10}};
    i32 r{v.visit([](const Foo& f) -> i32 { return f.value; },
                  [](const Bar&) -> i32 { return -1; },
                  [](const Baz&) -> i32 { return -2; })};
    CHECK(r == 10);
}

TEST_CASE("Variant::visit variadic lambda form with void return") {
    FBB  v{Bar{"side effect"}};
    auto called{false};
    v.visit([](const Foo&) -> void {},
            [&](const Bar&) -> void { called = true; },
            [](const Baz&) -> void {});
    CHECK(called);
}

TEST_CASE("Variant::visit on const Variant") {
    const FBB v{Bar{"const visit"}};
    auto      len{v.visit([](const Foo&) -> usize { return 0UZ; },
                     [](const Bar& b) -> usize { return b.value.size(); },
                     [](const Baz&) -> usize { return 0UZ; })};
    CHECK(len == 11UZ);
}

TEST_CASE("Variant::operator==") {
    CHECK(FBB{Foo{1}} == FBB{Foo{1}});
    CHECK(FBB{Bar{"x"}} == FBB{Bar{"x"}});
    CHECK_FALSE(FBB{Foo{1}} == FBB{Foo{2}});
    CHECK_FALSE(FBB{Foo{1}} == FBB{Bar{"x"}});
}

TEST_CASE("Variant copy construction") {
    FBB a{Bar{"copy me"}};
    FBB b{a};
    CHECK(b.is<Bar>());
    CHECK(b.as<Bar>().value == "copy me");
    a.as<Bar>().value = "modified";
    CHECK(b.as<Bar>().value == "copy me");
}

TEST_CASE("Variant copy assignment") {
    FBB a{Foo{1}};
    FBB b{Bar{"x"}};
    b = a;
    CHECK(b.is<Foo>());
    CHECK(b.as<Foo>().value == 1);
}

TEST_CASE("Variant move construction") {
    FBB a{Bar{"move me"}};
    FBB b{std::move(a)};
    CHECK(b.is<Bar>());
    CHECK(b.as<Bar>().value == "move me");
}

TEST_CASE("Variant move assignment") {
    FBB a{Baz{3.14}};
    FBB b{Foo{0}};
    b = std::move(a);
    CHECK(b.is<Baz>());
    CHECK(b.as<Baz>().value == 3.14);
}

TEST_CASE("Variant copy/move destructor accounting") {
    Tracker::reset();
    {
        Variant<Tracker, Foo> a{Tracker{0}};
        CHECK(Tracker::live_count == 1);
        Variant<Tracker, Foo> b = a; // copy (increment)
        CHECK(Tracker::live_count == 2);
        Variant<Tracker, Foo> c = std::move(b); // move (no increment)
        CHECK(Tracker::live_count == 2);
    } // a and c destroyed
    CHECK(Tracker::live_count == 0);
}

} // namespace ghoti::tests
