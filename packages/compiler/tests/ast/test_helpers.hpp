#pragma once

#include <print>
#include <span>

#include <catch_amalgamated.hpp>

#include "ast/statements/expression.hpp"

namespace conch::tests::helpers {

template <ast::LeafNode To, typename From> auto try_into(const From& node) -> const To& {
    REQUIRE(node.template is<To>());
    return ast::Node::as<To>(node);
}

template <typename N>
auto into_expression_statement(const N& node) -> const ast::ExpressionStatement& {
    return try_into<ast::ExpressionStatement>(node);
}

template <typename E>
auto check_errors(std::span<const E> actual, std::span<const E> expected = {}) {
    if (expected.empty()) {
        if (!actual.empty()) { std::println("{}", actual); }
        REQUIRE(actual.empty());
        return;
    }

    REQUIRE(actual.size() == expected.size());
}

} // namespace conch::tests::helpers
