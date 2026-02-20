#include <print>

#include <catch_amalgamated.hpp>

#include "ast/expressions/test_identifiers.hpp"
#include "ast/test_helpers.hpp"

#include "lexer/keywords.hpp"

#include "parser/parser.hpp"

#include "ast/expressions/identifier.hpp"

namespace conch::tests {

namespace helpers {

auto test_ident(std::string_view input, Optional<TokenType> expected_type) -> void {
    Parser p{input};
    auto   parse_result = p.consume();

    if (!parse_result.second.empty()) {
        std::println("{}", parse_result.second);
        REQUIRE(parse_result.second.empty());
    }
    REQUIRE(parse_result.first.size() == 1);

    auto&       ast = parse_result.first;
    const auto  actual{std::move(ast[0])};
    const auto& expr_stmt = helpers::into_expression_statement(*actual);

    const ast::IdentifierExpression& expected{Token{*expected_type, input}};
    REQUIRE(expected == expr_stmt.get_expression());
}

} // namespace helpers

TEST_CASE("Normal identifiers") {
    helpers::test_ident("foobar");
    helpers::test_ident("int", TokenType::INT_TYPE);
    helpers::test_ident("uint", TokenType::UINT_TYPE);
    helpers::test_ident("float", TokenType::FLOAT_TYPE);
    helpers::test_ident("byte", TokenType::BYTE_TYPE);
    helpers::test_ident("string", TokenType::STRING_TYPE);
    helpers::test_ident("bool", TokenType::BOOL_TYPE);
    helpers::test_ident("void", TokenType::VOID_TYPE);
}

TEST_CASE("Builtin identifiers") {
    for (const auto& [str, tok] : ALL_BUILTINS) { helpers::test_ident(str, tok); }
}

} // namespace conch::tests
