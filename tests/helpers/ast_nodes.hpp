#pragma once

#include <string_view>
#include <variant>

#include <catch_amalgamated.hpp>

#include "parser/parser.hpp"

#include "ast/node.hpp"
#include "ast/statements/expression.hpp"

namespace helpers {

using namespace conch;

template <typename N>
concept Node = requires { typename N::value_type; };

template <typename N>
auto into_expression_statement(const N& node) -> const ast::ExpressionStatement& {
    REQUIRE(node.get_kind() == ast::NodeKind::EXPRESSION_STATEMENT);
    return static_cast<const ast::ExpressionStatement&>(node);
}

template <Node N>
auto test_primitive(std::string_view                                       input,
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

    auto&       ast = parse_result.first;
    const auto  actual{std::move(ast[0])};
    const auto& expr_stmt = into_expression_statement(*actual);
    const N     expected{Token{*expected_type, node_token_slice}, std::get<T>(expected_value)};
    REQUIRE(expected == expr_stmt.get_expression());
}

template <Node N>
auto test_primitive(std::string_view                                       input,
                    Optional<TokenType>                                    expected_type,
                    std::variant<typename N::value_type, ParserDiagnostic> expected_value) -> void {
    test_primitive<N>(input, input, expected_type, expected_value);
}

auto test_ident(std::string_view input, Optional<TokenType> expected_type = TokenType::IDENT)
    -> void;

} // namespace helpers
