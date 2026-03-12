#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "ast/helpers.hpp"

#include "ast/expressions/prefix.hpp"

#include "lexer/operators.hpp"

namespace conch::tests {

namespace helpers {

template <ast::LeafNode N> auto test_prefix_expr(const Operator& op) -> void {
    const auto input = fmt::format("{}a;", op.first);
    test_expr_stmt(input, N{Token{op}, make_ident("a")});
}

} // namespace helpers

TEST_CASE("Unary expressions") {
    helpers::test_prefix_expr<ast::UnaryExpression>(operators::BANG);
    helpers::test_prefix_expr<ast::UnaryExpression>(operators::NOT);
    helpers::test_prefix_expr<ast::UnaryExpression>(operators::MINUS);
    helpers::test_prefix_expr<ast::UnaryExpression>(operators::PLUS);
}

TEST_CASE("Dereference expression") {
    helpers::test_prefix_expr<ast::DereferenceExpression>(operators::STAR);
}

TEST_CASE("Reference expressions") {
    helpers::test_prefix_expr<ast::ReferenceExpression>(operators::BW_AND);
    helpers::test_prefix_expr<ast::ReferenceExpression>(operators::AND_MUT);
}

TEST_CASE("Implicit access expression") {
    helpers::test_prefix_expr<ast::ImplicitAccessExpression>(operators::DOT);
}

TEST_CASE("Prefix without operand") {
    helpers::test_fail("!", ParserDiagnostic{ParserError::PREFIX_MISSING_OPERAND, 1, 1});
    helpers::test_fail("!;",
                       ParserDiagnostic{"No prefix parse function for SEMICOLON(;) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        2});
}

} // namespace conch::tests
