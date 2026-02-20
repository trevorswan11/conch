#pragma once

#include <string_view>
#include <variant>

#include <catch_amalgamated.hpp>

#include "ast/test_helpers.hpp"

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
    auto [ast, errors] = p.consume();

    if (std::holds_alternative<ParserDiagnostic>(expected_value)) {
        REQUIRE(errors.size() == 1);
        const auto& actual_error = errors[0];
        REQUIRE(std::get<ParserDiagnostic>(expected_value) == actual_error);
        return;
    }

    REQUIRE(errors.empty());
    REQUIRE(ast.size() == 1);

    const auto  actual{std::move(ast[0])};
    const auto& expr_stmt = into_expression_statement(*actual);
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
