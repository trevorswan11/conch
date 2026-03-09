#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/if.hpp"

namespace conch::tests {

TEST_CASE("If without alternate") {
    helpers::test_expr_stmt(
        "if (a) { b; };",
        ast::IfExpression{Token{keywords::IF},
                          helpers::make_ident("a"),
                          helpers::make_expr_block_stmt(helpers::ident_from("b")),
                          {}});

    helpers::test_expr_stmt("if (a) b;",
                            ast::IfExpression{Token{keywords::IF},
                                              helpers::make_ident("a"),
                                              helpers::make_expr_stmt(helpers::ident_from("b")),
                                              {}});
}

TEST_CASE("If with alternate") {
    helpers::test_expr_stmt(
        "if (a) { b; } else { c; };",
        ast::IfExpression{Token{keywords::IF},
                          helpers::make_ident("a"),
                          helpers::make_expr_block_stmt(helpers::ident_from("b")),
                          helpers::make_expr_block_stmt(helpers::ident_from("c"))});

    helpers::test_expr_stmt(
        "if (a) b; else { c; };",
        ast::IfExpression{Token{keywords::IF},
                          helpers::make_ident("a"),
                          helpers::make_expr_stmt(helpers::ident_from("b")),
                          helpers::make_expr_block_stmt(helpers::ident_from("c"))});
}

TEST_CASE("If without condition") {
    helpers::test_fail("if () b;", ParserDiagnostic{ParserError::IF_MISSING_CONDITION, 1, 1});
}

TEST_CASE("If with illegal consequence") {
    helpers::test_fail("if (a) import std;",
                       ParserDiagnostic{ParserError::ILLEGAL_IF_BRANCH, 1, 8});
}

TEST_CASE("If with illegal alternate") {
    helpers::test_fail("if (a) {} else import std;",
                       ParserDiagnostic{ParserError::ILLEGAL_IF_BRANCH, 1, 16});
}

} // namespace conch::tests
