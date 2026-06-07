#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "diagnostic.hh"
#include "option.hh"
#include "types.hh"

namespace ghoti {

struct SomethingLocationed {};
struct SomethingElseLocationed {};

namespace traits {

template <> struct SourceInfo<SomethingLocationed> {
    static auto get(const SomethingLocationed&) noexcept -> SourceLocation { return {0, 42}; }
};

template <> struct SourceInfo<SomethingElseLocationed> {
    static auto get(const SomethingElseLocationed&) noexcept -> SourceLocation { return {42, 0}; }
};

} // namespace traits

namespace tests {

enum class TestEnum : u8 {
    SAD,
    MAD,
};

TEST_CASE("Diagnostic traits") {
    STATIC_CHECK(traits::DiagnosticType<Diagnostic<TestEnum>>);
    STATIC_CHECK_FALSE(traits::DiagnosticType<TestEnum>);
}

TEST_CASE("Location and error only") {
    SomethingLocationed  l;
    Diagnostic<TestEnum> d{TestEnum::SAD, l};
    CHECK("error: SAD 1:43" == d.to_string(opt::none, false));
}

TEST_CASE("Custom locateable") {
    SomethingLocationed  l;
    Diagnostic<TestEnum> d{"message", TestEnum::SAD, l};
    CHECK("error: message 1:43" == d.to_string(opt::none, false));
}

TEST_CASE("Error messages with associated files") {
    Diagnostic<TestEnum> d{"message", TestEnum::SAD};
    CHECK("foo.gh: error: message" == d.to_string("foo.gh", false));
}

TEST_CASE("Locateable Error messages with associated files") {
    SomethingLocationed  l;
    Diagnostic<TestEnum> d{"message", TestEnum::SAD, l};
    CHECK("foo.gh:1:43: error: message" == d.to_string("foo.gh", false));
}

TEST_CASE("Move constructor with new error") {
    SomethingLocationed  l;
    Diagnostic<TestEnum> d1{"message", TestEnum::SAD, l};
    Diagnostic<TestEnum> d2{std::move(d1), TestEnum::MAD};
    CHECK("error: message 1:43" == d2.to_string(opt::none, false));
}

TEST_CASE("Move constructor with new location") {
    SomethingLocationed  l;
    Diagnostic<TestEnum> d1{"message", TestEnum::SAD, l};

    SomethingElseLocationed e;
    Diagnostic<TestEnum>    d2{std::move(d1), e};
    CHECK("error: message 43:1" == d2.to_string(opt::none, false));
}

} // namespace tests

} // namespace ghoti
