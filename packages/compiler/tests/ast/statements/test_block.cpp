#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/primitive.hpp"

namespace conch::tests {

TEST_CASE("Single statement blocks") {
    helpers::test_stmt("{a;};", helpers::expr_block_stmt_from(helpers::ident_from("a")));
    helpers::test_stmt("{2;};",
                       helpers::expr_block_stmt_from(
                           ast::SignedIntegerExpression{Token{TokenType::INT_10, "2"}, 2}));
}

TEST_CASE("Multiple statement block") {
    helpers::test_stmt("{ a; b; 2; c; };",
                       helpers::expr_block_stmt_from(
                           helpers::ident_from("a"),
                           helpers::ident_from("b"),
                           ast::SignedIntegerExpression{Token{TokenType::INT_10, "2"}, 2},
                           helpers::ident_from("c")));
}

TEST_CASE("Empty block") {
    helpers::test_stmt("{};", helpers::expr_block_stmt_from());
    helpers::test_stmt("{}", helpers::expr_block_stmt_from());
}

TEST_CASE("Non-terminated block") {
    helpers::test_fail(
        "{ ",
        ParserDiagnostic{"Expected token RBRACE, found END", ParserError::UNEXPECTED_TOKEN, 1, 3});
}

TEST_CASE("Illegal block inner statement") {
    helpers::test_fail("{ import std; };",
                       ParserDiagnostic{ParserError::ILLEGAL_BLOCK_STATEMENT, 1, 3});
}

} // namespace conch::tests
