#pragma once

#include <string_view>
#include <type_traits>

#include <catch_amalgamated.hpp>

#include "parser/parser.hpp"

#include "ast/expressions/primitive.hpp"
#include "ast/node.hpp"
#include "ast/statements/expression.hpp"

namespace helpers {

using namespace conch;

template <typename T>
auto numbers(std::string_view input, TokenType expected_type, T expected_value) -> void {
    Parser p{input};
    auto   parse_result = p.consume();

    REQUIRE(parse_result.second.empty());
    REQUIRE(parse_result.first.size() == 1);

    using NodeType =
        std::conditional_t<std::is_same_v<T, f64>,
                           ast::FloatExpression,
                           std::conditional_t<std::is_same_v<T, usize>,
                                              ast::SizeIntegerExpression,
                                              std::conditional_t<std::is_same_v<T, u64>,
                                                                 ast::UnsignedIntegerExpression,
                                                                 ast::SignedIntegerExpression>>>;

    auto&      ast = parse_result.first;
    const auto actual{std::move(ast[0])};
    REQUIRE(actual->get_kind() == ast::NodeKind::EXPRESSION_STATEMENT);
    const auto&    expr_stmt = static_cast<const ast::ExpressionStatement&>(*actual);
    const NodeType expected{Token{expected_type, input}, expected_value};
    REQUIRE(expected == expr_stmt.get_expression());
}

} // namespace helpers
