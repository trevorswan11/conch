#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/type.hpp"
#include "ast/statements/block.hpp"

namespace conch::tests {

using Parameters = std::vector<ast::FunctionParameter>;

namespace helpers {

auto function_expr_from(Optional<ast::SelfParameter>&&     self,
                        Parameters&&                       parameters,
                        ast::ExplicitType&&                return_type,
                        Optional<Box<ast::BlockStatement>> block = nullopt)
    -> ast::FunctionExpression {
    return ast::FunctionExpression{Token{keywords::FN},
                                   std::move(self),
                                   std::move(parameters),
                                   std::move(return_type),
                                   std::move(block)};
}

auto function_expr_from(Optional<ast::SelfParameter>&&     self,
                        ast::ExplicitType&&                return_type,
                        Optional<Box<ast::BlockStatement>> block = nullopt)
    -> ast::FunctionExpression {
    return function_expr_from(std::move(self), {}, std::move(return_type), std::move(block));
}

auto function_expr_from(Parameters&&                       parameters,
                        ast::ExplicitType&&                return_type,
                        Optional<Box<ast::BlockStatement>> block = nullopt)
    -> ast::FunctionExpression {
    return function_expr_from(
        nullopt, std::move(parameters), std::move(return_type), std::move(block));
}

auto function_expr_from(ast::ExplicitType&&                return_type,
                        Optional<Box<ast::BlockStatement>> block = nullopt)
    -> ast::FunctionExpression {
    return function_expr_from(nullopt, {}, std::move(return_type), std::move(block));
}

} // namespace helpers

namespace mods = helpers::type_modifiers;

TEST_CASE("Function without body, self, or parameters") {
    helpers::test_expr_stmt(
        "fn(): int;",
        helpers::function_expr_from(ast::ExplicitType{mods::BASE, helpers::make_ident("int")}));
}

TEST_CASE("Function with self but no parameters or body") {
    helpers::test_expr_stmt(
        "fn(&self): int;",
        helpers::function_expr_from(ast::SelfParameter{mods::REF, helpers::make_ident("self")},
                                    ast::ExplicitType{mods::BASE, helpers::make_ident("int")}));
}

TEST_CASE("Function with parameters but no self or body") {
    helpers::test_expr_stmt(
        "fn(a: A, b: *B): int;",
        helpers::function_expr_from(
            helpers::make_parameters(
                ast::FunctionParameter{helpers::make_ident("a"),
                                       ast::ExplicitType{mods::BASE, helpers::make_ident("A")}},
                ast::FunctionParameter{helpers::make_ident("b"),
                                       ast::ExplicitType{mods::PTR, helpers::make_ident("B")}}),
            ast::ExplicitType{mods::BASE, helpers::make_ident("int")}));
}

TEST_CASE("Function with self & parameters but no body") {
    helpers::test_expr_stmt(
        "fn(&mut this, a: A, b: *B): int;",
        helpers::function_expr_from(
            ast::SelfParameter{mods::MUT_REF, helpers::make_ident("this")},
            helpers::make_parameters(
                ast::FunctionParameter{helpers::make_ident("a"),
                                       ast::ExplicitType{mods::BASE, helpers::make_ident("A")}},
                ast::FunctionParameter{helpers::make_ident("b"),
                                       ast::ExplicitType{mods::PTR, helpers::make_ident("B")}}),
            ast::ExplicitType{mods::BASE, helpers::make_ident("int")}));
}

TEST_CASE("Full function expression") {
    helpers::test_expr_stmt(
        "fn(*mut this, a: A, b: *B, ): int { c; };",
        helpers::function_expr_from(
            ast::SelfParameter{mods::MUT_PTR, helpers::make_ident("this")},
            helpers::make_parameters(
                ast::FunctionParameter{helpers::make_ident("a"),
                                       ast::ExplicitType{mods::BASE, helpers::make_ident("A")}},
                ast::FunctionParameter{helpers::make_ident("b"),
                                       ast::ExplicitType{mods::PTR, helpers::make_ident("B")}}),
            ast::ExplicitType{mods::BASE, helpers::make_ident("int")},
            helpers::make_expr_block_stmt(helpers::ident_from("c"))));
}

TEST_CASE("Function missing return type") {
    helpers::test_fail(
        "fn(*mut this, a: A, b: *B, );",
        ParserDiagnostic{
            "Expected token COLON, found SEMICOLON", ParserError::UNEXPECTED_TOKEN, 1, 29});

    helpers::test_fail("fn(*mut this, a: A, b: *B, ): ;",
                       ParserDiagnostic{"No prefix parse function for SEMICOLON(;) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        31});
}

TEST_CASE("Function parameter missing type") {
    helpers::test_fail(
        "fn(*mut this, a): int;",
        ParserDiagnostic{
            "Expected token COLON, found RPAREN", ParserError::UNEXPECTED_TOKEN, 1, 16});
}

TEST_CASE("Out-of-place self parameter") {
    helpers::test_fail("fn(a: A, &self): int;",
                       ParserDiagnostic{ParserError::ILLEGAL_IDENTIFIER, 1, 10});

    helpers::test_fail(
        "fn(a: A, self): int;",
        ParserDiagnostic{
            "Expected token COLON, found RPAREN", ParserError::UNEXPECTED_TOKEN, 1, 14});
}

TEST_CASE("Default function parameter") {
    helpers::test_fail("fn(a: A = 2): int;",
                       ParserDiagnostic{ParserError::FUNCTION_PARAMETER_HAS_DEFAULT_VALUE, 1, 5});
}

TEST_CASE("Non-terminated parameter list") {
    helpers::test_fail("fn(a: A, : int;", ParserDiagnostic{ParserError::ILLEGAL_IDENTIFIER, 1, 10});
}

} // namespace conch::tests
