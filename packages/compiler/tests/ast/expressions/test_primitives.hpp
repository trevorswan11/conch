#pragma once

#include <string_view>
#include <variant>

#include <catch_amalgamated.hpp>

#include "parser/parser.hpp"

#include "ast/expressions/primitive.hpp"
#include "ast/statements/expression.hpp"

namespace conch::tests::helpers {

template <ast::PrimitiveNode N>
auto primitive(std::string_view                                       input,
               std::string_view                                       node_token_slice,
               Optional<TokenType>                                    expected_type,
               std::variant<typename N::value_type, ParserDiagnostic> expected_value) -> void {
    using T = typename N::value_type;
    Parser p{input};
    auto   parse_result = p.consume();

    if (std::holds_alternative<ParserDiagnostic>(expected_value)) {
        const auto& actual_errors = parse_result.second;
        REQUIRE(actual_errors.size() == 1);
        const auto& actual_error = actual_errors[0];
        REQUIRE(std::get<ParserDiagnostic>(expected_value) == actual_error);
        return;
    }

    REQUIRE(parse_result.second.empty());
    REQUIRE(parse_result.first.size() == 1);

    auto&      ast = parse_result.first;
    const auto actual{std::move(ast[0])};
    REQUIRE(actual->is<ast::ExpressionStatement>());
    const auto& expr_stmt = ast::Node::as<ast::ExpressionStatement>(*actual);
    const N     expected{Token{*expected_type, node_token_slice}, std::get<T>(expected_value)};
    REQUIRE(expected == expr_stmt.get_expression());
}

template <ast::PrimitiveNode N>
auto primitive(std::string_view                                       input,
               Optional<TokenType>                                    expected_type,
               std::variant<typename N::value_type, ParserDiagnostic> expected_value) -> void {
    primitive<N>(input, input, expected_type, expected_value);
}

} // namespace conch::tests::helpers
