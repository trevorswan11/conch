#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/for.hpp"
#include "ast/expressions/infix.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/statements/jump.hpp"

namespace conch::tests {

TEST_CASE("For loop with base captures") {
    helpers::test_expr_stmt(
        "for (arr) |i| { a; };",
        ast::ForLoopExpression{
            Token{keywords::FOR},
            helpers::make_vector<Box<ast::Expression>>(helpers::make_ident("arr")),
            helpers::make_vector<ast::ForLoopCapture>(
                ast::ForLoopCapture::Valued{{}, helpers::make_ident("i")}),
            helpers::make_expr_block_stmt(helpers::ident_from("a")),
            {}});
}

TEST_CASE("For loop with modified captures") {
    helpers::test_expr_stmt(
        "for (arr, l, p) |i, &mut j, _| { a; };",
        ast::ForLoopExpression{
            Token{keywords::FOR},
            helpers::make_vector<Box<ast::Expression>>(
                helpers::make_ident("arr"), helpers::make_ident("l"), helpers::make_ident("p")),
            helpers::make_vector<ast::ForLoopCapture>(
                ast::ForLoopCapture::Valued{{}, helpers::make_ident("i")},
                ast::ForLoopCapture::Valued{ast::TypeModifier{ast::TypeModifier::Modifier::MUT_REF},
                                            helpers::make_ident("j")},
                ast::ForLoopCapture{}),
            helpers::make_expr_block_stmt(helpers::ident_from("a")),
            {}});
}

TEST_CASE("Full for loop with else") {
    helpers::test_expr_stmt(
        "for (0..4) |i| { a; } else return b;",
        ast::ForLoopExpression{
            Token{keywords::FOR},
            helpers::make_vector<Box<ast::Expression>>(make_box<ast::RangeExpression>(
                Token{TokenType::INT_10, "0"},
                make_box<ast::SignedIntegerExpression>(Token{TokenType::INT_10, "0"}, 0),
                TokenType::DOT_DOT,
                make_box<ast::SignedIntegerExpression>(Token{TokenType::INT_10, "4"}, 4))),
            helpers::make_vector<ast::ForLoopCapture>(
                ast::ForLoopCapture::Valued{{}, helpers::make_ident("i")}),
            helpers::make_expr_block_stmt(helpers::ident_from("a")),
            make_box<ast::JumpStatement>(Token{keywords::RETURN}, helpers::make_ident("b"))});
}

TEST_CASE("Non-terminated iterables") {
    helpers::test_fail("for (0..4 |i| { a; } else return b;",
                       ParserDiagnostic{"No prefix parse function for LBRACE({) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        15},
                       ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        20});
}

TEST_CASE("Missing iterables") {
    helpers::test_fail(
        "for |i| { a; } else return b;",
        ParserDiagnostic{"Expected token LPAREN, found BW_OR", ParserError::UNEXPECTED_TOKEN, 1, 5},
        ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                         ParserError::MISSING_PREFIX_PARSER,
                         1,
                         14});
}

TEST_CASE("Non-terminated captures") {
    helpers::test_fail(
        "for (0..4) |i { a; } else return b;",
        ParserDiagnostic{
            "Expected token COMMA, found LBRACE", ParserError::UNEXPECTED_TOKEN, 1, 15},
        ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                         ParserError::MISSING_PREFIX_PARSER,
                         1,
                         20});
}

TEST_CASE("Missing captures") {
    helpers::test_fail(
        "for (0..4) { a; } else return b;",
        ParserDiagnostic{
            "Expected token BW_OR, found LBRACE", ParserError::UNEXPECTED_TOKEN, 1, 12},
        ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                         ParserError::MISSING_PREFIX_PARSER,
                         1,
                         17});
}

TEST_CASE("Illegal capture") {
    helpers::test_fail("for (0..4) |2| { a; } else return b;",
                       ParserDiagnostic{ParserError::ILLEGAL_IDENTIFIER, 1, 13},
                       ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        21});
}

TEST_CASE("Iterable-capture mismatch") {
    helpers::test_fail("for (0..4) |i, j| { a; } else return b;",
                       ParserDiagnostic{ParserError::FOR_ITERABLE_CAPTURE_MISMATCH, 1, 1});
}

TEST_CASE("Empty for block") {
    helpers::test_fail("for (0..4) |i| {} else return b;",
                       ParserDiagnostic{ParserError::EMPTY_FOR_LOOP, 1, 16});
}

TEST_CASE("Illegal for-else clause") {
    helpers::test_fail("for (0..4) |i| { a; } else import std;",
                       ParserDiagnostic{ParserError::ILLEGAL_LOOP_NON_BREAK, 1, 28});
}

} // namespace conch::tests
