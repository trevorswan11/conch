#include <array>
#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"
#include "syntax/error.hh"

namespace ghoti::tests {

TEST_CASE("Function type restrictions") {
    constexpr auto illegal_inputs = std::to_array<std::string_view>({
        "var a: &fn(): void;",
        "var a: &mut fn(): void;",
    });

    const auto expected_diag = [] {
        return syntax::Diagnostic{"Functions types may only be values or pointers",
                                  syntax::Error::ILLEGAL_FUNCTION_TYPE_MODIFIER,
                                  std::pair{0uz, 7uz}};
    };

    for (const auto& illegal : illegal_inputs) {
        helpers::test_parser_fail(illegal, expected_diag());
    }
}

TEST_CASE("Bodied function type") {
    helpers::test_parser_fail("var a: *mut fn(): void { b; };",
                              syntax::Diagnostic{"Function types may not have a body",
                                                 syntax::Error::EXPLICIT_FN_TYPE_HAS_BODY,
                                                 std::pair{0uz, 12uz}},
                              syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 28uz}});
}

TEST_CASE("Function return type restrictions") {
    helpers::test_parser_fail("var a: fn(): &void;",
                              syntax::Diagnostic{"Explicit `void` type cannot have a modifier",
                                                 syntax::Error::ILLEGAL_VOID_TYPE_MODIFIER,
                                                 std::pair{0uz, 13uz}});

    helpers::test_parser_fail("var a: fn(): &type;",
                              syntax::Diagnostic{"Explicit `type` type cannot have a modifier",
                                                 syntax::Error::ILLEGAL_TYPE_TYPE_MODIFIER,
                                                 std::pair{0uz, 13uz}});

    helpers::test_parser_fail("var a: fn(): &noreturn;",
                              syntax::Diagnostic{"Explicit `noreturn` type cannot have a modifier",
                                                 syntax::Error::ILLEGAL_NORETURN_TYPE_MODIFIER,
                                                 std::pair{0uz, 13uz}});
}

} // namespace ghoti::tests
