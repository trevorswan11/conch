#include <catch_amalgamated.hpp>

#include "ast/test_helpers.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/expressions/type.hpp"
#include "ast/statements/decl.hpp"
#include "lexer/keywords.hpp"
#include "lexer/operators.hpp"

namespace conch::tests {

TEST_CASE("Implicit declarations") {
    Parser p{"const a := 2;"};
    auto [ast, errors] = p.consume();

    REQUIRE(ast.size() == 1);
    helpers::check_errors(errors);

    const auto  actual{std::move(ast[0])};
    const auto& actual_decl = helpers::try_into<ast::DeclStatement>(*actual);

    const ast::DeclStatement expected{
        Token{keywords::CONST},
        make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "a"}),
        make_box<ast::TypeExpression>(Token{operators::WALRUS}, nullopt),
        make_box<ast::SignedIntegerExpression>(Token{TokenType::INT_10, "2"}, 2),
        ast::DeclModifiers::CONSTANT,
    };
    REQUIRE(expected == actual_decl);
}

TEST_CASE("Explicit declarations") {
    Parser p{"var a: int = 2;"};
    auto [ast, errors] = p.consume();

    REQUIRE(ast.size() == 1);
    helpers::check_errors(errors);

    const auto  actual{std::move(ast[0])};
    const auto& actual_decl = helpers::try_into<ast::DeclStatement>(*actual);

    const ast::DeclStatement expected{
        Token{keywords::VAR},
        make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "a"}),
        make_box<ast::TypeExpression>(
            Token{TokenType::COLON, ":"},
            ast::ExplicitType{
                {},
                ast::ExplicitTypeVariant{make_box<ast::IdentifierExpression>(Token{keywords::INT})},
                true,
            }),
        make_box<ast::SignedIntegerExpression>(Token{TokenType::INT_10, "2"}, 2),
        ast::DeclModifiers::VARIABLE,
    };
    REQUIRE(expected == actual_decl);
}

} // namespace conch::tests
