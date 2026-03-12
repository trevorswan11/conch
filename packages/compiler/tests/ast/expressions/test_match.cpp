#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/match.hpp"

namespace conch::tests {

TEST_CASE("Match without alternate") {
    helpers::test_expr_stmt(
        "match (a) { b => c; };",
        ast::MatchExpression{
            Token{keywords::MATCH},
            helpers::make_ident("a"),
            helpers::make_vector<ast::MatchArm>(ast::MatchArm{
                helpers::make_ident("b"), {}, helpers::make_expr_stmt(helpers::ident_from("c"))}),
            {}});

    helpers::test_expr_stmt(
        "match (a) { b => c; d => e; };",
        ast::MatchExpression{Token{keywords::MATCH},
                             helpers::make_ident("a"),
                             helpers::make_vector<ast::MatchArm>(
                                 ast::MatchArm{helpers::make_ident("b"),
                                               {},
                                               helpers::make_expr_stmt(helpers::ident_from("c"))},
                                 ast::MatchArm{helpers::make_ident("d"),
                                               {},
                                               helpers::make_expr_stmt(helpers::ident_from("e"))}),
                             {}});
}

TEST_CASE("Match with arm capture") {
    helpers::test_expr_stmt(
        "match (a) { b => |c| d; e => |_| f; g => h; };",
        ast::MatchExpression{Token{keywords::MATCH},
                             helpers::make_ident("a"),
                             helpers::make_vector<ast::MatchArm>(
                                 ast::MatchArm{helpers::make_ident("b"),
                                               helpers::make_ident("c"),
                                               helpers::make_expr_stmt(helpers::ident_from("d"))},
                                 ast::MatchArm{helpers::make_ident("e"),
                                               std::monostate{},
                                               helpers::make_expr_stmt(helpers::ident_from("f"))},
                                 ast::MatchArm{helpers::make_ident("g"),
                                               {},
                                               helpers::make_expr_stmt(helpers::ident_from("h"))}),
                             {}});
}

TEST_CASE("Match with alternate") {
    helpers::test_expr_stmt(
        "match (a) { b => { c; } } else d;",
        ast::MatchExpression{Token{keywords::MATCH},
                             helpers::make_ident("a"),
                             helpers::make_vector<ast::MatchArm>(ast::MatchArm{
                                 helpers::make_ident("b"),
                                 {},
                                 helpers::make_expr_block_stmt(helpers::ident_from("c"))}),
                             helpers::make_expr_stmt(helpers::ident_from("d"))});
}

TEST_CASE("Match without condition") {
    helpers::test_fail("match () { b => c; };",
                       ParserDiagnostic{ParserError::MATCH_EXPR_MISSING_CONDITION, 1, 1},
                       ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        20});

    helpers::test_fail(
        "match { b => c; };",
        ParserDiagnostic{
            "Expected token LPAREN, found LBRACE", ParserError::UNEXPECTED_TOKEN, 1, 7},
        ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                         ParserError::MISSING_PREFIX_PARSER,
                         1,
                         17});
}

TEST_CASE("Armless match expression") {
    helpers::test_fail("match (a) {};", ParserDiagnostic{ParserError::ARMLESS_MATCH_EXPR, 1, 1});
}

TEST_CASE("Malformed arm LHS") {
    helpers::test_fail(
        "match {  => c; };",
        ParserDiagnostic{
            "Expected token LPAREN, found LBRACE", ParserError::UNEXPECTED_TOKEN, 1, 7},
        ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                         ParserError::MISSING_PREFIX_PARSER,
                         1,
                         16});
}

TEST_CASE("Illegal match arm rhs") {
    helpers::test_fail("match (a) { b => import std; };",
                       ParserDiagnostic{ParserError::ILLEGAL_MATCH_ARM, 1, 18});
}

TEST_CASE("Arm missing fat arrow") {
    helpers::test_fail(
        "match (a) { b c; };",
        ParserDiagnostic{
            "Expected token FAT_ARROW, found IDENT", ParserError::UNEXPECTED_TOKEN, 1, 15},
        ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                         ParserError::MISSING_PREFIX_PARSER,
                         1,
                         18});
}

TEST_CASE("Illegal match alternate") {
    helpers::test_fail("match (a) { b => c; } else import std;",
                       ParserDiagnostic{ParserError::ILLEGAL_MATCH_CATCH_ALL, 1, 28});
}

} // namespace conch::tests
