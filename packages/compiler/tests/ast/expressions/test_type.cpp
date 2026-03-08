#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "ast/helpers.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/expressions/type.hpp"
#include "ast/expressions/type_modifiers.hpp"
#include "ast/statements/block.hpp" // IWYU pragma: keep
#include "ast/statements/declaration.hpp"

#include "lexer/keywords.hpp"
#include "lexer/token.hpp"
#include "parser/parser.hpp"

namespace conch::tests {

const Token colon{TokenType::COLON, ":"};

namespace helpers {

auto test_type_expr(std::string_view type_str, ast::ExplicitType&& expected) -> void {
    const auto input = fmt::format("var a: {};", type_str);
    helpers::test_stmt(
        input,
        ast::DeclStatement{Token{keywords::VAR},
                           make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "a"}),
                           make_box<ast::TypeExpression>(colon, std::move(expected)),
                           nullopt,
                           ast::DeclModifiers::VARIABLE});
}

} // namespace helpers

using Parameters = std::vector<ast::FunctionParameter>;

TEST_CASE("Indent type") {
    helpers::test_type_expr(
        "int",
        ast::ExplicitType{{},
                          make_box<ast::IdentifierExpression>(Token{TokenType::INT_TYPE, "int"})});

    helpers::test_type_expr(
        "*int",
        ast::ExplicitType{ast::TypeModifier{ast::TypeModifier::Modifier::PTR},
                          make_box<ast::IdentifierExpression>(Token{TokenType::INT_TYPE, "int"})});
}

TEST_CASE("Function types") {
    helpers::test_type_expr(
        "fn(): noreturn",
        ast::ExplicitType{{},
                          make_box<ast::FunctionExpression>(
                              Token{keywords::FN},
                              nullopt,
                              Parameters{},
                              make_box<ast::TypeExpression>(
                                  colon,
                                  ast::ExplicitType{{},
                                                    make_box<ast::IdentifierExpression>(
                                                        Token{TokenType::NORETURN, "noreturn"})}),
                              nullopt)});

    helpers::test_type_expr(
        "fn(&self): *mut E",
        ast::ExplicitType{
            {},
            make_box<ast::FunctionExpression>(
                Token{keywords::FN},
                ast::SelfParameter{
                    ast::TypeModifier{ast::TypeModifier::Modifier::REF},
                    make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "self"})},
                Parameters{},
                make_box<ast::TypeExpression>(
                    colon,
                    ast::ExplicitType{
                        ast::TypeModifier{ast::TypeModifier::Modifier::MUT_PTR},
                        make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "E"})}),
                nullopt)});
}

TEST_CASE("Array types") {
    helpers::test_type_expr(
        "[5uz]*ulong",
        ast::ExplicitType{
            {},
            ast::ExplicitArrayType{
                make_box<ast::USizeIntegerExpression>(Token{TokenType::UZINT_10, "5uz"}, 5uz),
                make_box<ast::ExplicitType>(
                    ast::TypeModifier{ast::TypeModifier::Modifier::PTR},
                    make_box<ast::IdentifierExpression>(Token{TokenType::ULONG_TYPE, "ulong"}))}});
}

TEST_CASE("Recursive types") {
    helpers::test_type_expr(
        "&[S]&*mut T",
        ast::ExplicitType{
            ast::TypeModifier{ast::TypeModifier::Modifier::REF},
            ast::ExplicitArrayType{
                make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "S"}),
                make_box<ast::ExplicitType>(
                    ast::TypeModifier{ast::TypeModifier::Modifier::REF},
                    make_box<ast::ExplicitType>(
                        ast::TypeModifier{ast::TypeModifier::Modifier::MUT_PTR},
                        make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "T"})))}});
}

TEST_CASE("Complex function type (holistic)") {
    Parameters parameters;
    parameters.emplace_back(
        make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "b"}),
        make_box<ast::TypeExpression>(
            colon,
            ast::ExplicitType{ast::TypeModifier{ast::TypeModifier::Modifier::MUT_PTR},
                              make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "B"})}));
    helpers::test_type_expr(
        "*fn(&a, b: *mut B): &[0x2uz][N]*E",
        ast::ExplicitType{
            ast::TypeModifier{ast::TypeModifier::Modifier::PTR},
            make_box<ast::FunctionExpression>(
                Token{keywords::FN},
                ast::SelfParameter{
                    ast::TypeModifier{ast::TypeModifier::Modifier::REF},
                    make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "a"})},
                std::move(parameters),
                make_box<ast::TypeExpression>(
                    colon,
                    ast::ExplicitType{
                        ast::TypeModifier{ast::TypeModifier::Modifier::REF},
                        ast::ExplicitArrayType{
                            make_box<ast::USizeIntegerExpression>(
                                Token{TokenType::UZINT_16, "0x2uz"}, 0x2uz),
                            make_box<ast::ExplicitType>(
                                ast::TypeModifier{},
                                ast::ExplicitArrayType{
                                    make_box<ast::IdentifierExpression>(
                                        Token{TokenType::IDENT, "N"}),
                                    make_box<ast::ExplicitType>(
                                        ast::TypeModifier{ast::TypeModifier::Modifier::PTR},
                                        make_box<ast::IdentifierExpression>(
                                            Token{TokenType::IDENT, "E"}))})}}),
                nullopt)});
}

TEST_CASE("Volatile is restricted to declarations") {
    helpers::test_fail("var a: volatile int;",
                       ParserDiagnostic{"No prefix parse function for VOLATILE(volatile) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        8});
}

TEST_CASE("Function type restrictions") {
    const auto illegals = std::to_array<std::string_view>({
        "var a: *mut fn(): void;",
        "var a: &fn(): void;",
        "var a: &mut fn(): void;",
    });
    for (const auto& illegal : illegals) {
        helpers::test_fail(illegal,
                           ParserDiagnostic{ParserError::ILLEGAL_FUNCTION_TYPE_MODIFIER, 1, 8});
    }
}

TEST_CASE("Function return type restrictions") {
    helpers::test_fail("var a: fn(): &void;",
                       ParserDiagnostic{ParserError::ILLEGAL_RETURN_TYPE_MODIFIER, 1, 14});
    helpers::test_fail("var a: fn(): &noreturn;",
                       ParserDiagnostic{ParserError::ILLEGAL_RETURN_TYPE_MODIFIER, 1, 14});
}

} // namespace conch::tests
