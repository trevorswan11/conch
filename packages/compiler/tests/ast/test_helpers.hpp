#pragma once

#include <span>
#include <string_view>

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

template <typename E>
auto check_errors(std::span<const E> actual, std::span<const E> expected = {}) {
    if (expected.empty()) {
        if (!actual.empty()) { fmt::println("{}", actual); }
        REQUIRE(actual.empty());
        return;
    }

    REQUIRE(actual.size() == expected.size());
}

constexpr auto trim_semicolons(std::string_view str) {
    return string::trim_right(str, [](byte b) { return b == ';'; });
}

} // namespace conch::tests::helpers
