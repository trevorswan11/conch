#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/infix.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/expressions/while.hpp"
#include "ast/statements/jump.hpp"

namespace conch::tests {

TEST_CASE("While without continuation or else") {
    helpers::test_expr_stmt(
        "while (true) {a;};",
        ast::WhileLoopExpression{Token{keywords::WHILE},
                                 make_box<ast::BoolExpression>(Token{keywords::TRUE}, true),
                                 {},
                                 helpers::make_expr_block_stmt(helpers::ident_from("a")),
                                 {}});
}

TEST_CASE("While with continuation") {
    helpers::test_expr_stmt(
        "while (true) : (i += 1) {a;};",
        ast::WhileLoopExpression{
            Token{keywords::WHILE},
            make_box<ast::BoolExpression>(Token{keywords::TRUE}, true),
            make_box<ast::AssignmentExpression>(
                Token{TokenType::IDENT, "i"},
                helpers::make_ident("i"),
                TokenType::PLUS_ASSIGN,
                make_box<ast::SignedIntegerExpression>(Token{TokenType::INT_10, "1"}, 1)),
            helpers::make_expr_block_stmt(helpers::ident_from("a")),
            {}});
}

TEST_CASE("While with else") {
    helpers::test_expr_stmt(
        "while (true) {a;} else return b;",
        ast::WhileLoopExpression{
            Token{keywords::WHILE},
            make_box<ast::BoolExpression>(Token{keywords::TRUE}, true),
            {},
            helpers::make_expr_block_stmt(helpers::ident_from("a")),
            make_box<ast::JumpStatement>(Token{keywords::RETURN}, helpers::make_ident("b"))});
}

TEST_CASE("Full while loop") {
    helpers::test_expr_stmt(
        "while (true) : (i += 1) {a;} else return b;",
        ast::WhileLoopExpression{
            Token{keywords::WHILE},
            make_box<ast::BoolExpression>(Token{keywords::TRUE}, true),
            make_box<ast::AssignmentExpression>(
                Token{TokenType::IDENT, "i"},
                helpers::make_ident("i"),
                TokenType::PLUS_ASSIGN,
                make_box<ast::SignedIntegerExpression>(Token{TokenType::INT_10, "1"}, 1)),
            helpers::make_expr_block_stmt(helpers::ident_from("a")),
            make_box<ast::JumpStatement>(Token{keywords::RETURN}, helpers::make_ident("b"))});
}

TEST_CASE("Empty while with continuation") {
    helpers::test_expr_stmt(
        "while (true) : (i += 1) {};",
        ast::WhileLoopExpression{
            Token{keywords::WHILE},
            make_box<ast::BoolExpression>(Token{keywords::TRUE}, true),
            make_box<ast::AssignmentExpression>(
                Token{TokenType::IDENT, "i"},
                helpers::make_ident("i"),
                TokenType::PLUS_ASSIGN,
                make_box<ast::SignedIntegerExpression>(Token{TokenType::INT_10, "1"}, 1)),
            helpers::make_block_stmt(),
            {}});
}

TEST_CASE("Empty while") {
    helpers::test_fail("while (true) {};", ParserDiagnostic{ParserError::EMPTY_WHILE_LOOP, 1, 14});
}

TEST_CASE("Missing while condition") {
    helpers::test_fail("while () {a;};",
                       ParserDiagnostic{ParserError::WHILE_MISSING_CONDITION, 1, 1},
                       ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        13});
}

TEST_CASE("Unclosed while body") {
    helpers::test_fail("while (true) {;",
                       ParserDiagnostic{"No prefix parse function for SEMICOLON(;) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        15});
}

TEST_CASE("Unclosed while condition") {
    helpers::test_fail(
        "while (true {};",
        ParserDiagnostic{
            "Expected token RPAREN, found LBRACE", ParserError::UNEXPECTED_TOKEN, 1, 13});
}

TEST_CASE("Malformed while continuation") {
    helpers::test_fail("while (true) : () {};",
                       ParserDiagnostic{ParserError::EMPTY_WHILE_CONTINUATION, 1, 12});

    helpers::test_fail(
        "while (true) : (i += 1 {};",
        ParserDiagnostic{
            "Expected token RPAREN, found LBRACE", ParserError::UNEXPECTED_TOKEN, 1, 24});
}

TEST_CASE("Illegal while-else clause") {
    helpers::test_fail("while (true) : (i += 1) {} else import std;",
                       ParserDiagnostic{ParserError::ILLEGAL_LOOP_NON_BREAK, 1, 33});

    helpers::test_fail("while (true) : (i += 1) {} else;",
                       ParserDiagnostic{"No prefix parse function for SEMICOLON(;) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        32});
}

} // namespace conch::tests
