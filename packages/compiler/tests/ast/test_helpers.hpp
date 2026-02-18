#pragma once

#include <catch_amalgamated.hpp>

#include "ast/statements/expression.hpp"

namespace conch::tests::helpers {

template <typename N>
auto into_expression_statement(const N& node) -> const ast::ExpressionStatement& {
    REQUIRE(node.template is<ast::ExpressionStatement>());
    return ast::Node::as<ast::ExpressionStatement>(node);
}

} // namespace conch::tests::helpers
