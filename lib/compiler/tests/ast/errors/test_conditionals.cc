#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"
#include "syntax/error.hh"

namespace ghoti::tests {

TEST_CASE("If without condition") {
    helpers::test_parser_fail("if () b;",
                              syntax::Diagnostic{"If expressions must have a condition",
                                                 syntax::Error::IF_MISSING_CONDITION,
                                                 std::pair{0uz, 0uz}});
}

TEST_CASE("If with illegal consequence") {
    helpers::test_parser_fail("if (a) import std;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_IF_BRANCH, 0, 7});
}

TEST_CASE("If with illegal alternate") {
    helpers::test_parser_fail("if (a) {} else import std;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_IF_BRANCH, 0, 15});
}

TEST_CASE("Match without condition") {
    helpers::test_parser_fail("match () { b => c, };",
                              syntax::Diagnostic{"Match expressions must have a condition",
                                                 syntax::Error::MATCH_EXPR_MISSING_CONDITION,
                                                 std::pair{0uz, 0uz}});

    helpers::test_parser_fail(
        "match { b => c, };",
        syntax::Diagnostic{
            "Expected token LPAREN, found LBRACE", syntax::Error::UNEXPECTED_TOKEN, 0, 6});
}

TEST_CASE("Armless match expression") {
    helpers::test_parser_fail("match (a) {};",
                              syntax::Diagnostic{"Match expressions must have at least one arm",
                                                 syntax::Error::ARMLESS_MATCH_EXPR,
                                                 std::pair{0uz, 0uz}});
}

TEST_CASE("Malformed arm pattern") {
    helpers::test_parser_fail(
        "match {  => c, };",
        syntax::Diagnostic{
            "Expected token LPAREN, found LBRACE", syntax::Error::UNEXPECTED_TOKEN, 0, 6});

    helpers::test_parser_fail(
        "match (a) { for (0..3) |i| { var a: i32; } => |b| c };",
        syntax::Diagnostic{"Unmatchable expression 'for loop' used as a match arm pattern",
                           syntax::Error::ILLEGAL_MATCH_PATTERN,
                           std::pair{0uz, 12uz}});
}

TEST_CASE("Illegal match arm dispatch") {
    helpers::test_parser_fail("match (a) { b => import std; };",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_MATCH_ARM, 0, 17});
}

TEST_CASE("Arm missing fat arrow") {
    helpers::test_parser_fail(
        "match (a) { b c, };",
        syntax::Diagnostic{
            "Expected token FAT_ARROW, found IDENT", syntax::Error::UNEXPECTED_TOKEN, 0, 14});
}

TEST_CASE("Arm separated by semicolon instead of comma") {
    helpers::test_parser_fail(
        "match (a) { b => c; c => d, };",
        syntax::Diagnostic{
            "Semicolon is not allowed in this context", syntax::Error::UNEXPECTED_TOKEN, 0, 18});
}

TEST_CASE("Illegal match catch-all") {
    helpers::test_parser_fail(
        "match (a) { b => c, _ => f, _ => g, };",
        syntax::Diagnostic{
            "Duplicate catch-all match arm", syntax::Error::ILLEGAL_MATCH_CATCH_ALL, 0, 28});

    helpers::test_parser_fail(
        "match (a) { _ => |b| c, };",
        syntax::Diagnostic{"Catch-all match arms may not have a capture clause",
                           syntax::Error::ILLEGAL_MATCH_CATCH_ALL,
                           std::pair{0uz, 18uz}});
}

} // namespace ghoti::tests
