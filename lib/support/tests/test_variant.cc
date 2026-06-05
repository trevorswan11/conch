#include <string>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "helpers/raii_tracker.hh"
#include "types.hh"
#include "variant.hh"

// These tests were written with Claude following the advice of a coworker
// Generally, I do not like using LLMs for test writing, but we sometimes do it at
// work so I thought I'd give it a fair shot!

namespace ghoti::tests {

struct Foo {
    i32  value;
    bool operator==(const Foo&) const = default;
};

struct Bar {
    std::string value;
    bool        operator==(const Bar&) const = default;
};

struct Baz {
    f64  value;
    bool operator==(const Baz&) const = default;
};

using Tracker = helpers::RAIITracker;
using FBB     = Variant<Foo, Bar, Baz>;

TEST_CASE("Variant default construction activates first alternative") {
    Variant<Foo, Bar> v;
    CHECK(v.is<Foo>());
    CHECK(v.index() == 0uz);
}

TEST_CASE("Variant implicit construction from alternative type") {
    FBB v = Foo{42};
    CHECK(v.is<Foo>());
    CHECK(v.get<Foo>().value == 42);
}

TEST_CASE("Variant in-place construction") {
    FBB v{std::in_place_type<Bar>, "hello"};
    CHECK(v.is<Bar>());
    CHECK(v.get<Bar>().value == "hello");
}

TEST_CASE("Variant::emplace<T> changes active alternative") {
    FBB v = Foo{1};
    v.emplace<Bar>("emplaced");
    CHECK(v.is<Bar>());
    CHECK(v.get<Bar>().value == "emplaced");
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
    FBB v = Bar{"x"};
    CHECK(v.is<Bar>());
    CHECK_FALSE(v.is<Foo>());
    CHECK_FALSE(v.is<Baz>());
}

TEST_CASE("Variant::index") {
    CHECK(FBB{Foo{}}.index() == 0uz);
    CHECK(FBB{Bar{}}.index() == 1uz);
    CHECK(FBB{Baz{}}.index() == 2uz);
}

TEST_CASE("Variant::get<T> returns mutable reference") {
    FBB v              = Foo{1};
    v.get<Foo>().value = 99;
    CHECK(v.get<Foo>().value == 99);
}

TEST_CASE("Variant::get<T> returns const reference on const Variant") {
    const FBB  v = Bar{"const"};
    const Bar& b = v.get<Bar>();
    CHECK(b.value == "const");
}

TEST_CASE("Variant::get<T> moves out of rvalue Variant") {
    FBB v = Bar{"moved"};
    Bar b = std::move(v).get<Bar>();
    CHECK(b.value == "moved");
}

TEST_CASE("Variant::get_opt<T> returns engaged Option for active type") {
    FBB  v   = Baz{1.5};
    auto opt = v.get_opt<Baz>();
    REQUIRE(opt.has_value());
    CHECK(opt->value == 1.5);
}

TEST_CASE("Variant::get_opt<T> returns empty Option for inactive type") {
    FBB v = Foo{1};
    CHECK_FALSE(v.get_opt<Bar>().has_value());
}

TEST_CASE("Variant::get_opt<T> const yields const ref") {
    const FBB               v   = Bar{"ro"};
    opt::Option<const Bar&> opt = v.get_opt<Bar>();
    REQUIRE(opt.has_value());
    CHECK(opt->value == "ro");
}

TEST_CASE("Variant::get_opt<T> supports transform()") {
    FBB  v   = Bar{"transform"};
    auto len = v.get_opt<Bar>().transform([](const Bar& b) { return b.value.size(); });
    REQUIRE(len.has_value());
    CHECK(*len == 9uz);
}

TEST_CASE("Variant::visit variadic lambda form with non-void return") {
    FBB v = Foo{10};
    i32 r = v.visit([](const Foo& f) { return f.value; },
                    [](const Bar&) { return -1; },
                    [](const Baz&) { return -2; });
    CHECK(r == 10);
}

TEST_CASE("Variant::visit variadic lambda form with void return") {
    FBB  v      = Bar{"side effect"};
    bool called = false;
    v.visit([](const Foo&) {}, [&](const Bar&) { called = true; }, [](const Baz&) {});
    CHECK(called);
}

TEST_CASE("Variant::visit on const Variant") {
    const FBB v   = Bar{"const visit"};
    auto      len = v.visit([](const Foo&) { return 0uz; },
                       [](const Bar& b) { return b.value.size(); },
                       [](const Baz&) { return 0uz; });
    CHECK(len == 11uz);
}

TEST_CASE("Variant::visit Overloaded form still works (backward compat)") {
    FBB v   = Baz{2.71};
    f64 val = v.visit(Overloaded{[](const Foo&) { return 0.0; },
                                 [](const Bar&) { return 0.0; },
                                 [](const Baz& z) { return z.value; }});
    CHECK(val == 2.71);
}

TEST_CASE("Variant::operator==") {
    CHECK(FBB{Foo{1}} == FBB{Foo{1}});
    CHECK(FBB{Bar{"x"}} == FBB{Bar{"x"}});
    CHECK_FALSE(FBB{Foo{1}} == FBB{Foo{2}});
    CHECK_FALSE(FBB{Foo{1}} == FBB{Bar{"x"}});
}

TEST_CASE("Variant copy construction") {
    FBB a = Bar{"copy me"};
    FBB b = a;
    CHECK(b.is<Bar>());
    CHECK(b.get<Bar>().value == "copy me");
    a.get<Bar>().value = "modified";
    CHECK(b.get<Bar>().value == "copy me");
}

TEST_CASE("Variant copy assignment") {
    FBB a = Foo{1};
    FBB b = Bar{"x"};
    b     = a;
    CHECK(b.is<Foo>());
    CHECK(b.get<Foo>().value == 1);
}

TEST_CASE("Variant move construction") {
    FBB a = Bar{"move me"};
    FBB b = std::move(a);
    CHECK(b.is<Bar>());
    CHECK(b.get<Bar>().value == "move me");
}

TEST_CASE("Variant move assignment") {
    FBB a = Baz{3.14};
    FBB b = Foo{0};
    b     = std::move(a);
    CHECK(b.is<Baz>());
    CHECK(b.get<Baz>().value == 3.14);
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
