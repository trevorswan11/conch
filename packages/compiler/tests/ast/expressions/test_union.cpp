#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/union.hpp"

namespace conch::tests {

namespace mods = helpers::type_modifiers;

TEST_CASE("Correct union") {
    helpers::test_expr_stmt(
        "union { a: int, b: &mut T, };",
        ast::UnionExpression{
            Token{keywords::UNION},
            helpers::make_vector<ast::UnionField>(
                ast::UnionField{helpers::make_ident("a"),
                                ast::ExplicitType{mods::BASE, helpers::make_ident("int")}},
                ast::UnionField{helpers::make_ident("b"),
                                ast::ExplicitType{mods::MUT_REF, helpers::make_ident("T")}})});
}

TEST_CASE("Illegal union field name") {
    helpers::test_fail(
        "union { 2: int };",
        ParserDiagnostic{
            "Expected token IDENT, found INT_10", ParserError::UNEXPECTED_TOKEN, 1, 9});
}

TEST_CASE("Illegal union field type") {
    helpers::test_fail("union { a: 2 };",
                       ParserDiagnostic{ParserError::ILLEGAL_EXPLICIT_TYPE, 1, 10});
}

TEST_CASE("Empty union") {
    helpers::test_fail("union { };", ParserDiagnostic{ParserError::EMPTY_UNION, 1, 1});
}

} // namespace conch::tests
