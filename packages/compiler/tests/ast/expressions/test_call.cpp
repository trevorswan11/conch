#include <catch2/catch_test_macros.hpp>

#include "ast/expressions/prefix.hpp"
#include "ast/helpers.hpp"

#include "ast/expressions/call.hpp"
#include "ast/expressions/primitive.hpp"
#include "lexer/operators.hpp"

namespace conch::tests {

namespace mods = helpers::type_modifiers;

TEST_CASE("Call with no arguments") {
    const Token func{TokenType::IDENT, "func"};
    helpers::test_expr_stmt("func();", ast::CallExpression{func, helpers::make_ident(func), {}});
}

TEST_CASE("Trailing comma") {
    const Token func{keywords::builtins::SIN};
    helpers::test_expr_stmt(
        "@sin(23.6, );",
        ast::CallExpression{func,
                            helpers::make_ident(func),
                            helpers::make_vector<ast::CallArgument>(make_box<ast::DoubleExpression>(
                                Token{TokenType::DOUBLE, "23.6"}, 23.6))});
}

TEST_CASE("Builtin with multiple arguments") {
    const Token func{keywords::builtins::PTR_ADD};
    helpers::test_expr_stmt("@ptrAdd(a, 4uz);",
                            ast::CallExpression{func,
                                                helpers::make_ident(func),
                                                helpers::make_vector<ast::CallArgument>(
                                                    helpers::make_ident("a"),
                                                    make_box<ast::USizeIntegerExpression>(
                                                        Token{TokenType::UZINT_10, "4uz"}, 4uz))});
}

TEST_CASE("Type arguments in call") {
    const Token func{TokenType::IDENT, "a"};
    helpers::test_expr_stmt(
        "a(&mut r, t, *[N]int);",
        ast::CallExpression{
            func,
            helpers::make_ident(func),
            helpers::make_vector<ast::CallArgument>(
                make_box<ast::ReferenceExpression>(Token{operators::AND_MUT},
                                                   helpers::make_ident("r")),
                helpers::make_ident("t"),
                ast::ExplicitType{
                    mods::PTR,
                    ast::ExplicitArrayType{
                        helpers::make_ident("N"),
                        make_box<ast::ExplicitType>(mods::BASE, helpers::make_ident("int"))}})});
}

TEST_CASE("No arguments with comma") {
    helpers::test_fail("func(,)",
                       ParserDiagnostic{ParserError::COMMA_WITH_MISSING_CALL_ARGUMENT, 1, 6});
}

TEST_CASE("Non-comma separated arguments") {
    helpers::test_fail(
        "func(1 2)",
        ParserDiagnostic{
            "Expected token COMMA, found INT_10", ParserError::UNEXPECTED_TOKEN, 1, 8});
}

} // namespace conch::tests
