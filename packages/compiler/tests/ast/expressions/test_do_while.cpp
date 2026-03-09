#include <catch2/catch_test_macros.hpp>

#include "ast/expressions/do_while.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/helpers.hpp"

namespace conch::tests {

TEST_CASE("Correct do-while") {
    helpers::test_expr_stmt(
        "do {a;} while (true);",
        ast::DoWhileLoopExpression{Token{keywords::DO},
                                   helpers::make_expr_block_stmt(helpers::ident_from("a")),
                                   make_box<ast::BoolExpression>(Token{keywords::TRUE}, true)});
}

TEST_CASE("Empty do-while") {
    helpers::test_fail("do {} while (true);", ParserDiagnostic{ParserError::EMPTY_LOOP, 1, 4});
}

TEST_CASE("Missing do-while condition") {
    helpers::test_fail("do {a; } while ();",
                       ParserDiagnostic{ParserError::WHILE_MISSING_CONDITION, 1, 16});
}

TEST_CASE("Unclosed do-while body") {
    helpers::test_fail(
        "do { while (true);",
        ParserDiagnostic{
            "Expected token LBRACE, found SEMICOLON", ParserError::UNEXPECTED_TOKEN, 1, 18});
}

TEST_CASE("Unclosed do-while condition") {
    helpers::test_fail(
        "do {a; } while (true;",
        ParserDiagnostic{
            "Expected token RPAREN, found SEMICOLON", ParserError::UNEXPECTED_TOKEN, 1, 21});
}

} // namespace conch::tests
