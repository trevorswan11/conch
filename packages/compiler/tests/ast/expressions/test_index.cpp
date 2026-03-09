#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/expressions/index.hpp"
#include "ast/expressions/primitive.hpp"

namespace conch::tests {

TEST_CASE("Single-level index") {
    const Token arr{TokenType::IDENT, "arr"};
    helpers::test_expr_stmt("arr[3uz];",
                            ast::IndexExpression{arr,
                                                 make_box<ast::IdentifierExpression>(arr),
                                                 make_box<ast::USizeIntegerExpression>(
                                                     Token{TokenType::UZINT_10, "3uz"}, 3uz)});

    helpers::test_expr_stmt(
        "arr[i];",
        ast::IndexExpression{arr,
                             make_box<ast::IdentifierExpression>(arr),
                             make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "i"})});
}

TEST_CASE("Index on an index") {
    const Token arr{TokenType::IDENT, "arr"};
    helpers::test_expr_stmt(
        "arr[i][j];",
        ast::IndexExpression{arr,
                             make_box<ast::IndexExpression>(
                                 arr,
                                 make_box<ast::IdentifierExpression>(arr),
                                 make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "i"})),
                             make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "j"})});
}

TEST_CASE("No index") {
    helpers::test_fail("arr[]",
                       ParserDiagnostic{"No prefix parse function for RBRACKET(]) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        5});
}

} // namespace conch::tests
