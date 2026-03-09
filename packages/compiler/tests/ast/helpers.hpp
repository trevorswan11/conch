#pragma once

#include <algorithm>
#include <span>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "string.hpp"

#include "ast/statements/expression.hpp"

namespace conch::tests::helpers {

// Thin wrapper around Node is/as pattern with test assertion.
template <ast::LeafNode To, typename From> auto try_into(const From& node) -> const To& {
    REQUIRE(node.template is<To>());
    return ast::Node::as<To>(node);
}

template <typename N>
auto into_expression_statement(const N& node) -> const ast::ExpressionStatement& {
    return try_into<ast::ExpressionStatement>(node);
}

// Checks if the error list is empty, dumping the list's contents otherwise.
template <typename E> auto check_errors(std::span<const E> errors) {
    if (!errors.empty()) { fmt::println("{}", errors); }
    REQUIRE(errors.empty());
}

// Tests a failing input against the expected generated errors
template <usize N>
auto test_fail(std::string_view failing, std::array<ParserDiagnostic, N> expected_errors) -> void {
    Parser p{failing};
    auto [ast, errors] = p.consume();
    for (const auto& n : ast) { fmt::println("{}", *n); }
    REQUIRE(ast.empty());

    if (errors.size() != N) {
        for (const auto& e : errors) { fmt::println("{}", e); }
        REQUIRE(errors.size() == N);
    }
    REQUIRE(std::ranges::equal(errors, expected_errors));
}

// Helper for testing an input that is expected to generate only a single error
inline auto test_fail(std::string_view failing, ParserDiagnostic expected_error) -> void {
    test_fail<1>(failing, {expected_error});
}

constexpr auto trim_semicolons(std::string_view str) -> std::string_view {
    return string::trim_right(str, [](byte b) { return b == ';'; });
}

template <ast::LeafNode N> auto test_stmt(std::string_view input, const N& expected) -> void {
    Parser p{input};
    auto [ast, errors] = p.consume();

    helpers::check_errors<ParserDiagnostic>(errors);
    REQUIRE(ast.size() == 1);

    const auto  actual{std::move(ast[0])};
    const auto& actual_stmt = helpers::try_into<N>(*actual);
    REQUIRE(expected == actual_stmt);
}

template <ast::LeafNode N>
auto test_expr_stmt(std::string_view input, const Token& start_token, N&& expected) -> void {
    test_stmt(input, ast::ExpressionStatement{start_token, make_box<N>(std::move(expected))});
}

template <ast::LeafNode N> auto test_expr_stmt(std::string_view input, N&& expected) -> void {
    test_expr_stmt(input, expected.get_token(), std::move(expected));
}

template <typename T, typename... Ts> auto make_vector(Ts&&... es) -> std::vector<T> {
    std::vector<T> list;
    list.reserve(sizeof...(es));
    (list.emplace_back(std::forward<Ts>(es)), ...);
    return list;
}

} // namespace conch::tests::helpers
