#include <sstream>
#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include "helpers/common.hh"
#include "helpers/sema.hh"
#include "sema/error.hh"
#include "types.hh"

namespace ghoti::tests {

using MockFile = helpers::MockFile;

TEST_CASE("Resolving well-formed enum matching") {
    helpers::resolve_and_check("using E = enum { a }; match (E) { .a => 5 };");
    helpers::resolve_and_check("using E = enum { a, b }; match (E) { .a => 5, .b => 6 };");
    helpers::resolve_and_check("using E = enum { a, b }; match (E) { .a => 5, _ => 6 };");
    helpers::resolve_and_check("using E = enum { a, b, _ }; match (E) { .a => 5, _ => 6 };");
}

TEST_CASE("Resolving well-formed union matching") {
    helpers::resolve_and_check("using U = union { a: i32 }; match (U) { .a => 5 };");
    helpers::resolve_and_check(
        "using U = union { a: i32, b: i64 }; match (U) { .a => 5, .b => 4 };");
    helpers::resolve_and_check(
        "using U = union { a: i32, b: i64 }; match (U) { .a => 5, _ => 4 };");
}

TEST_CASE("Resolving well-formed builtin-type matching") {
    SECTION("Bools") {
        helpers::resolve_and_check("match (true) { true => {}, false => {} };");
        helpers::resolve_and_check("match (true) { true => {}, _ => {} };");
    }

    SECTION("Bytes") {
        std::stringstream arms;
        for (usize i = 0; i < 256; ++i) { fmt::print(arms, "{} => 0,", i); }
        const auto input = fmt::format("match('0') {{ {} }};", arms.view());
        helpers::resolve_and_check("match ('0') { 0 => {}, _ => {} };");
    }
}

TEST_CASE("Illegal resolved enum matcher type") {
    SECTION("Non-exhaustive") {
        helpers::test_resolver_fail(
            "using E = enum { a, _ }; match (E) { 3 => 5 };",
            sema::Diagnostic{
                "Match expressions over non-exhaustive enums must have a catch all arm",
                sema::Error::ILLEGAL_MATCH_PATTERN,
                std::pair{0uz, 25uz},
            });
    }

    SECTION("Duplicate enumerations") {
        const auto expected_diag = [](std::string_view suffix, usize col) {
            return sema::Diagnostic{
                fmt::format("Match expression contains duplicate enumeration{}", suffix),
                sema::Error::DUPLICATE_ENUMERATION,
                std::pair{0uz, col},
            };
        };

        helpers::test_resolver_fail("using E = enum { a }; match (E) { .a => 5, .a => 4 };",
                                    expected_diag(": a", 22uz));
        helpers::test_resolver_fail(
            "using E = enum { a, b }; match (E) { .a => 5, .a => 4, .b => 3, .b => 2 };",
            expected_diag("s: a, b", 25uz));
    }

    SECTION("Unknown enumerations") {
        const auto expected_diag = [](std::string_view suffix) {
            return sema::Diagnostic{
                fmt::format("Match expression contains unknown enumeration{}", suffix),
                sema::Error::UNKNOWN_ENUMERATION,
                std::pair{0uz, 22uz},
            };
        };

        helpers::test_resolver_fail("using E = enum { a }; match (E) { .a => 5, .b => 4 };",
                                    expected_diag(": b"));
        helpers::test_resolver_fail(
            "using E = enum { a }; match (E) { .a => 5, .b => 4, .c => 3 };",
            expected_diag("s: b, c"));
    }
}

TEST_CASE("Illegal resolved union matcher type") {
    SECTION("Pattern ast node restriction") {
        helpers::test_resolver_fail(
            "using U = union { a: i32 }; match (U) { 3 => 5 };",
            sema::Diagnostic{
                "Match arm may only have an implicit access pattern in this context",
                sema::Error::ILLEGAL_MATCH_PATTERN,
                std::pair{0uz, 40uz},
            });
    }

    SECTION("Duplicate fields") {
        const auto expected_diag = [](std::string_view suffix, usize col) {
            return sema::Diagnostic{
                fmt::format("Match expression contains duplicate union field{}", suffix),
                sema::Error::DUPLICATE_FIELD,
                std::pair{0uz, col},
            };
        };

        helpers::test_resolver_fail("using U = union { a: i32 }; match (U) { .a => 5, .a => 4 };",
                                    expected_diag(": a", 28uz));
        helpers::test_resolver_fail(
            "using U = union { a: i32, b: i64 }; match (U) { .a => 5, .a => 4, .b => 3, .b => 2 };",
            expected_diag("s: a, b", 36uz));
    }

    SECTION("Missing fields") {
        const auto expected_diag = [](std::string_view suffix, usize col) {
            return sema::Diagnostic{
                fmt::format("Match expression is non-exhaustive; missing union field{}", suffix),
                sema::Error::MISSING_FIELD,
                std::pair{0uz, col},
            };
        };

        helpers::test_resolver_fail("using U = union { a: i32, b: i64 }; match (U) { .a => 5 };",
                                    expected_diag(": b", 36uz));
        helpers::test_resolver_fail(
            "using U = union { a: i32, b: i64, c: f32 }; match (U) { .a => 5 };",
            expected_diag("s: b, c", 44uz));
    }

    SECTION("Unknown fields") {
        const auto expected_diag = [](std::string_view suffix) {
            return sema::Diagnostic{
                fmt::format("Match expression contains unknown union field{}", suffix),
                sema::Error::UNKNOWN_FIELD,
                std::pair{0uz, 28uz},
            };
        };

        helpers::test_resolver_fail("using U = union { a: i32 }; match (U) { .a => 5, .b => 4 };",
                                    expected_diag(": b"));
        helpers::test_resolver_fail(
            "using U = union { a: i32 }; match (U) { .a => 5, .b => 4, .c => 3 };",
            expected_diag("s: b, c"));
    }
}

TEST_CASE("Illegal match arms with primitives") {
    SECTION("Required catch-all") {
        const auto expected_diag = [](std::string_view kind) {
            return sema::Diagnostic{
                fmt::format("Matching on type '{}' requires a catch all arm with a pattern of '_'",
                            kind),
                sema::Error::TYPE_MISMATCH,
                std::pair{0uz, 7uz},
            };
        };

        helpers::test_resolver_fail("match (1) { 3 => 5 };", expected_diag("i32"));
        helpers::test_resolver_fail("match (1l) { 3 => 5 };", expected_diag("i64"));
        helpers::test_resolver_fail("match (1z) { 3 => 5 };", expected_diag("isize"));
        helpers::test_resolver_fail("match (1u) { 3 => 5 };", expected_diag("u32"));
        helpers::test_resolver_fail("match (1ul) { 3 => 5 };", expected_diag("u64"));
        helpers::test_resolver_fail("match (1uz) { 3 => 5 };", expected_diag("usize"));
    }

    SECTION("Optional catch-all") {
        const auto expected_diag = [](std::string_view kind, usize required_arms) {
            return sema::Diagnostic{
                fmt::format("Matching on type '{}' requires a catch all arm with "
                            "a pattern of '_' or exactly {} patterned arms",
                            kind,
                            required_arms),
                sema::Error::TYPE_MISMATCH,
                std::pair{0uz, 7uz},
            };
        };

        helpers::test_resolver_fail("match ('0') { 3 => 5 };", expected_diag("u8", 256));
        helpers::test_resolver_fail("match (false) { 3 => 5 };", expected_diag("bool", 2));
    }
}

TEST_CASE("Illegal resolved builtin matcher type") {
    SECTION("Floats") {
        const auto expected_diag = [] {
            return sema::Diagnostic{
                "Cannot match on floats due to precision; use an if statement with explicit "
                "precision handling",
                sema::Error::TYPE_MISMATCH,
                std::pair{0uz, 7uz},
            };
        };

        helpers::test_resolver_fail("match (2.3f) { 3 => 5 };", expected_diag());
        helpers::test_resolver_fail("match (2.3) { 3 => 5 };", expected_diag());
    }

    SECTION("Empty types") {
        const auto expected_diag = [] {
            return sema::Diagnostic{
                "Empty types cannot be matched on",
                sema::Error::TYPE_MISMATCH,
                std::pair{0uz, 7uz},
            };
        };

        helpers::test_resolver_fail("match ({}) { 3 => 5 };", expected_diag());
        helpers::test_resolver_fail("match (undefined) { 3 => 5 };", expected_diag());
    }

    SECTION("Other illegal builtin types") {
        helpers::test_resolver_fail(
            "using A = opaque; match (A) { 3 => 5 };",
            sema::Diagnostic{
                "Can only match on integers, bytes, and booleans; found 'opaque'",
                sema::Error::TYPE_MISMATCH,
                std::pair{0uz, 25uz},
            });
    }
}

TEST_CASE("Illegal resolved arbitrary matcher type") {
    const auto expected_diag = [](std::string_view kind, usize col) {
        return sema::Diagnostic{
            fmt::format("Can only match on enums, unions, and certain primitive types; found '{}'",
                        kind),
            sema::Error::TYPE_MISMATCH,
            std::pair{0uz, col},
        };
    };

    helpers::test_resolver_fail("match (@typeOf(i32)) { 3 => 5 };", expected_diag("type", 14));
    helpers::test_resolver_fail("match (^4) { 3 => 5 };", expected_diag("pointer", 7));
    helpers::test_resolver_fail("match (&mut 4) { 3 => 5 };", expected_diag("reference", 7));
    helpers::test_resolver_fail("var a: fn(): void; match (a) { 3 => 5 };",
                                expected_diag("function", 26));
    helpers::test_resolver_fail(
        "import std; match (std) { 3 => 5 };",
        helpers::make_vector<MockFile>(MockFile{"std.gh", "pub extern var a: i32;", "std"}),
        expected_diag("module", 19));
}

} // namespace ghoti::tests
