#pragma once

#include <string_view>
#include <type_traits>

#include "parser/parser.hpp"

#include "ast/expressions/primitive.hpp"

namespace helpers {

using namespace conch;

template <typename T>
auto numbers(std::string_view input, TokenType expected_type, T expected_value)
    -> bool {
    Parser p{input};
    auto   ast = p.consume();

    if (!ast.second.empty()) { return false; }
    if (ast.first.size() != 1) { return false; }

    using NodeType =
        std::conditional_t<std::is_same_v<T, f64>,
                           ast::FloatExpression,
                           std::conditional_t<std::is_same_v<T, usize>,
                                              ast::SizeIntegerExpression,
                                              std::conditional_t<std::is_same_v<T, u64>,
                                                                 ast::UnsignedIntegerExpression,
                                                                 ast::SignedIntegerExpression>>>;
    const auto     actual{std::move(ast.first[0])};
    const NodeType expected{Token{expected_type, input}, expected_value};
    return expected == *actual;
}

} // namespace helpers
