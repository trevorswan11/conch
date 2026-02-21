#include <initializer_list>
#include <sstream>
#include <string_view>

#include <catch_amalgamated.hpp>

#include "array.hpp"

#include "ast/statements/test_declarations.hpp"
#include "ast/test_helpers.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/expressions/type.hpp"
#include "ast/statements/decl.hpp"

#include "lexer/keywords.hpp"
#include "lexer/operators.hpp"

namespace conch::tests {

namespace helpers {

auto test_decl(std::string_view input, const ast::DeclStatement& expected) -> void {
    Parser p{input};
    auto [ast, errors] = p.consume();

    helpers::check_errors(errors);
    REQUIRE(ast.size() == 1);

    const auto  actual{std::move(ast[0])};
    const auto& actual_decl = helpers::try_into<ast::DeclStatement>(*actual);
    REQUIRE(expected == actual_decl);
}

} // namespace helpers

TEST_CASE("Explicit primitive declaration") {
    helpers::test_decl("var a: int = 2;",
                       ast::DeclStatement{
                           Token{keywords::VAR},
                           make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "a"}),
                           make_box<ast::TypeExpression>(
                               Token{TokenType::COLON, ":"},
                               ast::ExplicitType{
                                   {},
                                   ast::ExplicitTypeVariant{
                                       make_box<ast::IdentifierExpression>(Token{keywords::INT})},
                                   true,
                               }),
                           make_box<ast::SignedIntegerExpression>(Token{TokenType::INT_10, "2"}, 2),
                           ast::DeclModifiers::VARIABLE,
                       });
}

TEST_CASE("Explicit non-primitive declaration") {
    helpers::test_decl("var a: Foo = bar;",
                       ast::DeclStatement{
                           Token{keywords::VAR},
                           make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "a"}),
                           make_box<ast::TypeExpression>(
                               Token{TokenType::COLON, ":"},
                               ast::ExplicitType{
                                   {},
                                   ast::ExplicitTypeVariant{make_box<ast::IdentifierExpression>(
                                       Token{TokenType::IDENT, "Foo"})},
                                   false,
                               }),
                           make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "bar"}),
                           ast::DeclModifiers::VARIABLE,
                       });
}

TEST_CASE("Implicit comptime declaration") {
    helpers::test_decl(
        "comptime SIZE := 2uz;",
        ast::DeclStatement{
            Token{keywords::COMPTIME},
            make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "SIZE"}),
            make_box<ast::TypeExpression>(Token{operators::WALRUS}, nullopt),
            make_box<ast::USizeIntegerExpression>(Token{TokenType::UZINT_10, "2uz"}, 2uz),
            ast::DeclModifiers::COMPTIME,
        });
}

TEST_CASE("Correct declaration modifiers") {
    const auto test = [](std::initializer_list<Keyword> modifiers,
                         ast::DeclModifiers             flags,
                         bool                           initialized = true) {
        std::stringstream ss;
        for (const auto& keyword : modifiers) { ss << keyword.first << " "; }
        if (initialized) {
            ss << " a := 2;";
            helpers::test_decl(
                ss.view(),
                ast::DeclStatement{
                    Token{*modifiers.begin()},
                    make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "a"}),
                    make_box<ast::TypeExpression>(Token{operators::WALRUS}, nullopt),
                    make_box<ast::SignedIntegerExpression>(Token{TokenType::INT_10, "2"}, 2),
                    flags,
                });
        } else {
            ss << " a: int;";
            helpers::test_decl(
                ss.view(),
                ast::DeclStatement{
                    Token{*modifiers.begin()},
                    make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "a"}),
                    make_box<ast::TypeExpression>(
                        Token{TokenType::COLON, ":"},
                        ast::ExplicitType{
                            {},
                            ast::ExplicitTypeVariant{
                                make_box<ast::IdentifierExpression>(Token{keywords::INT})},
                            true,
                        }),
                    nullopt,
                    flags,
                });
        }
    };

    // Modifiers are order independent
    test({keywords::EXPORT, keywords::CONST},
         ast::DeclModifiers::EXPORT | ast::DeclModifiers::CONSTANT);
    test({keywords::PRIVATE, keywords::VAR},
         ast::DeclModifiers::PRIVATE | ast::DeclModifiers::VARIABLE);
    test({keywords::VAR, keywords::EXPORT},
         ast::DeclModifiers::VARIABLE | ast::DeclModifiers::EXPORT);

    // Uninitialized declarations are selective
    test({keywords::VAR}, ast::DeclModifiers::VARIABLE, false);
    test({keywords::EXTERN, keywords::CONST},
         ast::DeclModifiers::EXTERN | ast::DeclModifiers::CONSTANT,
         false);
    test({keywords::VAR, keywords::EXTERN},
         ast::DeclModifiers::VARIABLE | ast::DeclModifiers::EXTERN,
         false);
    test({keywords::VAR, keywords::PRIVATE},
         ast::DeclModifiers::VARIABLE | ast::DeclModifiers::PRIVATE,
         false);
}

static auto test_decl_fail(std::initializer_list<Keyword> modifiers,
                           ParserDiagnostic               expected_error,
                           std::string_view               init = "a := 2;") -> void {
    std::stringstream ss;
    for (const auto& keyword : modifiers) { ss << keyword.first << " "; }
    ss << init;

    Parser p{ss.view()};
    auto [ast, errors] = p.consume();
    for (const auto& n : ast) { std::println("{}", *n); }
    REQUIRE(ast.empty());

    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0] == expected_error);
}

TEST_CASE("Mutability restrictions") {
    const auto contending_mut = std::array{keywords::COMPTIME, keywords::VAR, keywords::CONST};
    for (const auto& mut : array::combinations(contending_mut)) {
        test_decl_fail({mut.first, mut.second},
                       ParserDiagnostic{ParserError::ILLEGAL_DECL_MODIFIERS, 1, 1});
    }
    test_decl_fail({keywords::COMPTIME, keywords::VAR, keywords::CONST},
                   ParserDiagnostic{ParserError::ILLEGAL_DECL_MODIFIERS, 1, 1});
}

TEST_CASE("Comptime restrictions") {
    const auto contending_mut = std::array{keywords::EXTERN, keywords::COMPTIME};
    for (const auto& mut : array::combinations(contending_mut)) {
        test_decl_fail({mut.first, mut.second},
                       ParserDiagnostic{ParserError::ILLEGAL_DECL_MODIFIERS, 1, 1});
    }
    test_decl_fail({keywords::EXTERN, keywords::COMPTIME},
                   ParserDiagnostic{ParserError::ILLEGAL_DECL_MODIFIERS, 1, 1});
}

TEST_CASE("ABI/Linkage restrictions") {
    const auto contending_mut = std::array{keywords::EXTERN, keywords::EXPORT};
    for (const auto& mut : array::combinations(contending_mut)) {
        test_decl_fail({mut.first, mut.second},
                       ParserDiagnostic{ParserError::ILLEGAL_DECL_MODIFIERS, 1, 1});
    }
    test_decl_fail({keywords::EXTERN, keywords::EXPORT},
                   ParserDiagnostic{ParserError::ILLEGAL_DECL_MODIFIERS, 1, 1});
}

TEST_CASE("Access restrictions") {
    const auto contending_access =
        std::array{keywords::PRIVATE, keywords::EXTERN, keywords::EXPORT};
    for (const auto& mut : array::combinations(contending_access)) {
        test_decl_fail({mut.first, mut.second},
                       ParserDiagnostic{ParserError::ILLEGAL_DECL_MODIFIERS, 1, 1});
    }
    test_decl_fail({keywords::PRIVATE, keywords::EXTERN, keywords::EXPORT},
                   ParserDiagnostic{ParserError::ILLEGAL_DECL_MODIFIERS, 1, 1});
}

TEST_CASE("Extern requirements") {
    test_decl_fail({keywords::EXTERN, keywords::CONST},
                   ParserDiagnostic{ParserError::EXTERN_VALUE_INITIALIZED, 1, 1});
    test_decl_fail({keywords::EXTERN, keywords::VAR},
                   ParserDiagnostic{ParserError::EXTERN_VALUE_INITIALIZED, 1, 1});
}

TEST_CASE("Constant requirements") {
    test_decl_fail({keywords::CONST},
                   ParserDiagnostic{ParserError::CONST_DECL_MISSING_VALUE, 1, 1},
                   "a: int;");
    test_decl_fail({keywords::COMPTIME},
                   ParserDiagnostic{ParserError::CONST_DECL_MISSING_VALUE, 1, 1},
                   "a: int;");
}

} // namespace conch::tests
