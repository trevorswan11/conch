#pragma once

#include <algorithm>
#include <span>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "string.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/statements/block.hpp"
#include "ast/statements/expression.hpp"

#include "lexer/keywords.hpp"

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
template <typename... Ds>
    requires(std::same_as<Ds, ParserDiagnostic> && ...)
auto test_fail(std::string_view failing, Ds&&... expected_diagnostics) -> void {
    Parser p{failing};
    auto [ast, errors] = p.consume();
    for (const auto& n : ast) { fmt::println("{}", *n); }
    REQUIRE(ast.empty());

    std::array expected_arr{std::forward<Ds>(expected_diagnostics)...};
    const auto expected_count = sizeof...(Ds);

    if (errors.size() != expected_count) {
        for (const auto& e : errors) { fmt::println("{}", e); }
        REQUIRE(errors.size() == expected_count);
    }
    REQUIRE(std::ranges::equal(errors, expected_arr));
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
auto expr_stmt_from(const Token& start_token, N&& expected) -> ast::ExpressionStatement {
    return ast::ExpressionStatement{start_token, make_box<N>(std::move(expected))};
}

template <ast::LeafNode N> auto make_expr_stmt(N&& expected) -> Box<ast::ExpressionStatement> {
    return make_box<ast::ExpressionStatement>(
        expr_stmt_from(expected.get_token(), std::move(expected)));
}

template <ast::LeafNode N>
auto test_expr_stmt(std::string_view input, const Token& start_token, N&& expected) -> void {
    test_stmt(input, expr_stmt_from(start_token, std::move(expected)));
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

inline auto ident_from(std::string_view name) -> ast::IdentifierExpression {
    const auto extract = [](const auto& key) { return key.second; };
    const auto tt      = get_keyword(name).transform(extract).value_or(
        get_builtin(name).transform(extract).value_or(TokenType::IDENT));
    return ast::IdentifierExpression{Token{tt, name}};
}

inline auto ident_from(const Token& tok) -> ast::IdentifierExpression {
    return ast::IdentifierExpression{tok};
}

inline auto make_ident(std::string_view name) -> Box<ast::IdentifierExpression> {
    return make_box<ast::IdentifierExpression>(ident_from(name));
}

inline auto make_ident(const Token& tok) -> Box<ast::IdentifierExpression> {
    return make_box<ast::IdentifierExpression>(ident_from(tok));
}

// Creates a block statement by boxing all passed statements
template <ast::LeafNode... Ns> auto block_stmt_from(Ns&&... nodes) -> ast::BlockStatement {
    return ast::BlockStatement{
        Token{TokenType::LBRACE, "{"},
        make_vector<Box<ast::Statement>>(make_box<Ns>(std::forward<Ns>(nodes))...)};
}

// Creates a boxed block statement by boxing all passed statements
template <ast::LeafNode... Ns> auto make_block_stmt(Ns&&... nodes) -> Box<ast::BlockStatement> {
    return make_box<ast::BlockStatement>(block_stmt_from(std::forward<Ns>(nodes)...));
}

// Creates a block statement by packing all passed expressions into expression statements
template <ast::LeafNode... Ns> auto expr_block_stmt_from(Ns&&... nodes) -> ast::BlockStatement {
    return block_stmt_from(
        ast::ExpressionStatement{nodes.get_token(), make_box<Ns>(std::forward<Ns>(nodes))}...);
}

// Creates a boxed block statement by packing all passed expressions into expression statements
template <ast::LeafNode... Ns>
auto make_expr_block_stmt(Ns&&... nodes) -> Box<ast::BlockStatement> {
    return make_box<ast::BlockStatement>(expr_block_stmt_from(std::forward<Ns>(nodes)...));
}

template <typename... Ps>
    requires(std::same_as<Ps, ast::FunctionParameter> && ...)
auto make_parameters(Ps&&... params) -> std::vector<ast::FunctionParameter> {
    return make_vector<ast::FunctionParameter>(std::forward<Ps>(params)...);
}

} // namespace conch::tests::helpers
