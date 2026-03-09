#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/call.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp"

#include "lexer/keywords.hpp"

namespace conch::tests {

TEST_CASE("Call with no arguments") {
    const Token func{TokenType::IDENT, "func"};
    helpers::test_expr_stmt(
        "func();", ast::CallExpression{func, make_box<ast::IdentifierExpression>(func), {}});
}

TEST_CASE("Trailing comma") {
    const Token func{keywords::builtins::SIN};
    helpers::test_expr_stmt("@sin(23.6, );",
                            ast::CallExpression{func,
                                                make_box<ast::IdentifierExpression>(func),
                                                helpers::make_vector<Box<ast::Expression>>(
                                                    make_box<ast::FloatExpression>(
                                                        Token{TokenType::FLOAT, "23.6"}, 23.6))});
}

TEST_CASE("Builtin with multiple arguments") {
    const Token func{keywords::builtins::PTR_ADD};
    helpers::test_expr_stmt(
        "@ptrAdd(a, 4uz);",
        ast::CallExpression{
            func,
            make_box<ast::IdentifierExpression>(func),
            helpers::make_vector<Box<ast::Expression>>(
                make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "a"}),
                make_box<ast::USizeIntegerExpression>(Token{TokenType::UZINT_10, "4uz"}, 4uz))});
}

TEST_CASE("No arguments with comma") {
    helpers::test_fail("func(,)",
                       ParserDiagnostic{"No prefix parse function for COMMA(,) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        6});
}

TEST_CASE("Non-comma separated arguments") {
    helpers::test_fail(
        "func(1 2)",
        ParserDiagnostic{
            "Expected token COMMA, found INT_10", ParserError::UNEXPECTED_TOKEN, 1, 8});
}

} // namespace conch::tests
