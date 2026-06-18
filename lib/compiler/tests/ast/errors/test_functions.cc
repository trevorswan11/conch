#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <stdx/types.hh>

#include "helpers/ast.hh"
#include "syntax/error.hh"

namespace ghoti::tests {

TEST_CASE("Function missing return type") {
    helpers::test_parser_fail(
        "fn(^mut this, a: A, b: ^B, );",
        syntax::Diagnostic{
            "Expected token COLON, found SEMICOLON", syntax::Error::UNEXPECTED_TOKEN, 0, 28});

    helpers::test_parser_fail("fn(^mut this, a: A, b: ^B, ): ;",
                              syntax::Diagnostic{"No prefix parse function for SEMICOLON(;) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0UZ, 30UZ}});

    helpers::test_parser_fail("fn(^mut this, a: A, b: ^B, ): ",
                              syntax::Diagnostic{syntax::Error::MISSING_EXPLICIT_TYPE, 0, 28});
}

TEST_CASE("Function parameter missing type") {
    helpers::test_parser_fail(
        "fn(^mut this, a): i32;",
        syntax::Diagnostic{
            "Expected token COLON, found RPAREN", syntax::Error::UNEXPECTED_TOKEN, 0, 15});
}

TEST_CASE("Out-of-place self parameter") {
    helpers::test_parser_fail("fn(a: A, &self): i32;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_IDENTIFIER, 0, 9});

    helpers::test_parser_fail(
        "fn(a: A, self): i32;",
        syntax::Diagnostic{
            "Expected token COLON, found RPAREN", syntax::Error::UNEXPECTED_TOKEN, 0, 13});
}

TEST_CASE("Illegal self parameter modifier") {
    const auto test_illegal_self = [](std::string_view modifier) -> void {
        helpers::test_parser_fail(
            fmt::format("fn({} self): i32 {{}};", modifier),
            syntax::Diagnostic{
                "Self parameters cannot be marked volatile; they must be values, refs, or pointers",
                syntax::Error::ILLEGAL_SELF_PARAMETER_MODIFIER,
                std::pair{0UZ, 3UZ}});
    };

    test_illegal_self("volatile");
    test_illegal_self("mut_volatile");
}

TEST_CASE("Out-of-place variadic parameter") {
    helpers::test_parser_fail(
        "fn(a: A, ..., b: B): i32;",
        syntax::Diagnostic{
            "Expected token RPAREN, found IDENT", syntax::Error::UNEXPECTED_TOKEN, 0, 14});
}

TEST_CASE("Default function parameter") {
    helpers::test_parser_fail("fn(a: A = 2): i32;",
                              syntax::Diagnostic{"Function parameters may not have default values",
                                                 syntax::Error::FN_PARAMETER_HAS_DEFAULT_VALUE,
                                                 std::pair{0UZ, 6UZ}});
}

TEST_CASE("Noreturn function types") {
    helpers::test_parser_fail("fn(a: &noreturn): i32;",
                              syntax::Diagnostic{"Explicit `noreturn` type cannot have a modifier",
                                                 syntax::Error::ILLEGAL_NORETURN_TYPE_MODIFIER,
                                                 std::pair{0UZ, 6UZ}});

    helpers::test_parser_fail(
        "fn(a: noreturn): i32;",
        syntax::Diagnostic{"Function parameter types may not be marked `noreturn`",
                           syntax::Error::FN_PARAMETER_IS_NORETURN,
                           std::pair{0UZ, 6UZ}});

    helpers::test_parser_fail("fn(a: A): &noreturn;",
                              syntax::Diagnostic{"Explicit `noreturn` type cannot have a modifier",
                                                 syntax::Error::ILLEGAL_NORETURN_TYPE_MODIFIER,
                                                 std::pair{0UZ, 10UZ}});
}

TEST_CASE("Illegal type function types") {
    const auto expected_diag = [](usize col) -> syntax::Diagnostic {
        return {"Explicit `type` type cannot have a modifier",
                syntax::Error::ILLEGAL_TYPE_TYPE_MODIFIER,
                std::pair{0UZ, col}};
    };

    helpers::test_parser_fail("fn(A: &type): i32;", expected_diag(6));
    helpers::test_parser_fail("fn(A: type): &type;", expected_diag(13));
}

TEST_CASE("Non-terminated parameter list") {
    helpers::test_parser_fail("fn(a: A, : i32;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_IDENTIFIER, 0, 9});
}

} // namespace ghoti::tests
